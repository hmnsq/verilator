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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Force.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग ForceState final {
सार्वजनिक:
    संरचना ForceRange VL_NOT_FINAL {
        पूर्णाङ्क m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        पूर्णाङ्क m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        पूर्णाङ्क m_padLsb = 0;  // Bit positions for RHS padding
        पूर्णाङ्क m_padMsb = 0;  // Bit positions for RHS padding
    };

    संरचना ForceInfo final : ForceRange {
        // MEMBERS
        पूर्णाङ्क m_forceId = 0;  // Unique (per signal) variable of this force assignment
        तार्किक m_hasArraySel = मिच्छ;  // If this has an array select on LHS
        तार्किक m_isExternal = मिच्छ;  // Synthetic force for the public force controls
        तार्किक m_skipRdRefresh = मिच्छ;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_ग्रन्थिः_व्यञ्जकम्* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = स्वभाव;
        ForceInfo(पूर्णाङ्क rangeLsb, पूर्णाङ्क rangeMsb, पूर्णाङ्क padLsb, पूर्णाङ्क padMsb, पूर्णाङ्क forceId,
                  तार्किक hasArraySel, AstVarScope* rhsVarVscp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsExprp)
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

    संरचना VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, पूर्णाङ्क> m_forcePathToIndex;
        पूर्णाङ्क m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    गुप्त:
        स्थिर शून्य build_Force_Path_कुञ्चिका_Recurse(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, string& out) {
            जइ (VN_IS(nodep, VarRef)) प्रत्यागम;
            जइ (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_कुञ्चिका_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                प्रत्यागम;
            }
            जइ (AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = selp->fromp();
                build_Force_Path_कुञ्चिका_Recurse(fromp, out);
                out += "|T" + selp->name();
                प्रत्यागम;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        स्थिर string force_Path_कुञ्चिका(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
            string out;
            build_Force_Path_कुञ्चिका_Recurse(nodep, out);
            प्रत्यागम out;
        }

    सार्वजनिक:
        पूर्णाङ्क getOrCreateForcePathIndex(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
            ध्रुव auto pair
                = m_forcePathToIndex.emplace(force_Path_कुञ्चिका(nodep), m_nextForcePathIndex);
            जइ (pair.second) ++m_nextForcePathIndex;
            प्रत्यागम pair.first->second;
        }

        पूर्णाङ्क findForcePathIndex(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) ध्रुव {
            ध्रुव auto it = m_forcePathToIndex.find(force_Path_कुञ्चिका(nodep));
            प्रत्यागम it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    संरचना ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    संरचना ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        तार्किक m_hasBitSel = मिच्छ;  // Bit-select was found along the path too
    };

    संरचना ForceRangeInfo final : ForceRange {
        तार्किक m_hasArraySel = मिच्छ;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

गुप्त:
    using ScopeVarCache = std::unordered_map<ध्रुव AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ध्रुव VNUser1InUse m_user1InUse;
    ध्रुव VNUser2InUse m_user2InUse;

सार्वजनिक:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<ध्रुव AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<ध्रुव AstVarScope*>;

गुप्त:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, पूर्णाङ्क> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    तार्किक m_doingAssign = मिच्छ;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

सार्वजनिक:
    ForceState(तार्किक doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    स्थिर AstConst* makeZeroConst(Ast_ग्रन्थिः* nodep, पूर्णाङ्क width) {
        V3_सङ्ख्या zero{nodep, width};
        zero.setAllBits0();
        प्रत्यागम नूतन AstConst{nodep->fileline(), zero};
    }

    स्थिर AstConst* makeConst32(File_पङ्क्तिः* flp, पूर्णाङ्क मानम्) {
        प्रत्यागम नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, static_cast<uint32_t>(मानम्)};
    }

    स्थिर AstConst* makeRangeMaskConst(Ast_ग्रन्थिः* nodep, पूर्णाङ्क width, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        V3_सङ्ख्या mask{nodep, width};
        mask.setAllBits0();
        प्रति (पूर्णाङ्क bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        प्रत्यागम नूतन AstConst{nodep->fileline(), mask};
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* zeroPadToBaseWidth(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क baseWidth, पूर्णाङ्क padLsb,
                                           पूर्णाङ्क padMsb) {
        जइ (baseWidth <= 0) प्रत्यागम exprp;
        ध्रुव पूर्णाङ्क नीचः_Pad = padLsb;
        ध्रुव पूर्णाङ्क उच्चः_Pad = baseWidth - (padMsb + 1);
        जइ (नीचः_Pad > 0) {
            exprp = नूतन AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, नीचः_Pad)};
        }
        जइ (उच्चः_Pad > 0) {
            exprp = नूतन AstConcat{exprp->fileline(), makeZeroConst(exprp, उच्चः_Pad), exprp};
        }
        प्रत्यागम exprp;
    }

    स्थिर तार्किक isUnpackedArrayDType(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        प्रत्यागम VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    स्थिर तार्किक isBitwiseDType(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव AstBasicDType* ध्रुव basicp = nodep->dtypep()->skipRefp()->basicp();
        प्रत्यागम basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* cast_To_ग्रन्थिः_DType(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः* dtypeFromp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = dtypeFromp->dtypep()->skipRefp();
        ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp();
        जइ (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            प्रत्यागम exprp;
        }
        प्रत्यागम नूतन AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    स्थिर शून्य markNonReplaceable(AstVarRef* ध्रुव nodep) { nodep->user1SetOnce(); }
    तार्किक isNotReplaceable(ध्रुव AstVarRef* ध्रुव nodep) ध्रुव {
        प्रत्यागम nodep->user1() || m_permanentlyProtected.गणनम्(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    शून्य markPermanentlyProtected(AstVarRef* ध्रुव nodep) ध्रुव {
        markNonReplaceable(nodep);
        m_permanentlyProtected.निवेशय(nodep);
    }

    स्थिर std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        प्रति (auto& it : info.m_forces) forceps.प्रक्षिप_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ध्रुव ForceInfo* ap, ध्रुव ForceInfo* bp) {
            प्रत्यागम ap->m_forceId < bp->m_forceId;
        });
        प्रत्यागम forceps;
    }

    स्थिर std::vector<ध्रुव ForceInfo*> forceInfosInIdOrder(ध्रुव VarForceInfo& info) {
        std::vector<ध्रुव ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        प्रति (ध्रुव auto& it : info.m_forces) forceps.प्रक्षिप_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](ध्रुव ForceInfo* ap, ध्रुव ForceInfo* bp) {
            प्रत्यागम ap->m_forceId < bp->m_forceId;
        });
        प्रत्यागम forceps;
    }

    स्थिर तार्किक isOpaquePathSelector(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम VN_IS(nodep, Sel) || VN_IS(nodep, ग्रन्थिः_Sel) || VN_IS(nodep, StructSel);
    }

    स्थिर तार्किक isOutermostOpaquePathSelector(ध्रुव Ast_ग्रन्थिः* nodep) {
        जइ (!isOpaquePathSelector(nodep)) प्रत्यागम मिच्छ;
        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        प्रत्यागम !backp || !isOpaquePathSelector(backp);
    }

    स्थिर AstVarRef* getOneVarRef(Ast_ग्रन्थिः_व्यञ्जकम्* forceStmtp) {
        Ast_ग्रन्थिः* ध्रुव basep = forceStmtp->baseFromp(सच्च);
        जइ (AstSampled* sampledp = VN_CAST(basep, Sampled))
            जइ (Ast_ग्रन्थिः_व्यञ्जकम्* exprp = VN_CAST(sampledp->exprp(), ग्रन्थिः_व्यञ्जकम्))
                प्रत्यागम getOneVarRef(exprp);
        AstVarRef* ध्रुव varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        प्रत्यागम varRefp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* buildNestedArraySel(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp,
                                            ध्रुव std::vector<पूर्णाङ्क>& indicies) {
        Ast_ग्रन्थिः_व्यञ्जकम्* curp = fromp;
        प्रति (ध्रुव पूर्णाङ्क idx : indicies) curp = नूतन AstArraySel{flp, curp, idx};
        प्रत्यागम curp;
    }

    template <typename Fn>
    स्थिर Ast_ग्रन्थिः_Stmt* foreachUnpackedLeaf(ध्रुव std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_ग्रन्थिः_Stmt* headp = nullptr;
        Ast_ग्रन्थिः_Stmt* tailp = nullptr;
        जइ (dims.empty()) प्रत्यागम nullptr;
        पूर्णाङ्क total = 1;
        प्रति (ध्रुव AstUnpackArrayDType* ध्रुव d : dims) total *= d->elementsConst();
        जइ (total <= 0) प्रत्यागम nullptr;
        std::vector<पूर्णाङ्क> idx(dims.size(), 0);
        प्रति (पूर्णाङ्क flat = 0; flat < total; ++flat) {
            Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = buildLeaf(idx, flat);
            जइ (!headp) {
                headp = stmtp;
            } अण्णहा {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            प्रति (पूर्णाङ्क d = static_cast<पूर्णाङ्क>(dims.size()) - 1; d >= 0; --d) {
                जइ (++idx[d] < dims[d]->elementsConst()) भङ्ग;
                idx[d] = 0;
            }
        }
        प्रत्यागम headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, AstVar* varp,
                                     तार्किक requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        जइ (ध्रुव AstSel* ध्रुव outerSelp = VN_CAST(lhsp, Sel)) {
            पूर्णाङ्क totalLsb = 0;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* curp = lhsp; ध्रुव AstSel* ध्रुव selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                जइ (requireConstRangeSelect) {
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
        जइ (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<पूर्णाङ्क> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            प्रति (AstArraySel* ध्रुव selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.प्रक्षिप_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } अण्णहा जइ (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        जइ (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            ध्रुव पूर्णाङ्क index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        प्रत्यागम info;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* add_Rhs_मानम्_Reads(ध्रुव VarForceInfo& varInfo, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) ध्रुव {
        जइ (!doingAssign()) प्रत्यागम exprp;

        ध्रुव std::vector<ध्रुव ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        जइ (forceps.empty()) प्रत्यागम exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ध्रुव cexprp = नूतन AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        प्रति (ध्रुव ForceInfo* ध्रुव finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ध्रुव refp
                = नूतन AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        प्रत्यागम cexprp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceReadCall(ध्रुव VarForceInfo& varInfo, File_पङ्क्तिः* flp, VCMethod method,
                                     Ast_ग्रन्थिः_व्यञ्जकम्* originalExprp, Ast_ग्रन्थिः* dtypeFromp,
                                     Ast_ग्रन्थिः_व्यञ्जकम्* indexExprp) ध्रुव {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origValp
            = add_Rhs_मानम्_Reads(varInfo, cast_To_ग्रन्थिः_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        जइ (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        प्रत्यागम callp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceRdExpression(ध्रुव VarForceInfo& varInfo, File_पङ्क्तिः* flp) ध्रुव {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ध्रुव varp = varInfo.m_varVscp->varp();
        जइ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            जइ (doingAssign()) { प्रत्यागम नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सच्च);
            AstVarRef* ध्रुव baseRefp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            प्रत्यागम createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* ध्रुव baseRefp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enRefp = नूतन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valRefp = नूतन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        जइ (isBitwiseDType(varp)) {
            प्रत्यागम नूतन AstOr{
                flp, नूतन AstAnd{flp, enRefp, valRefp},
                नूतन AstAnd{flp, नूतन AstNot{flp, enRefp->clone_वृक्षः_Pure(मिच्छ)}, baseRefp}};
        }
        प्रत्यागम नूतन AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_ग्रन्थिः_Stmt* createForceRdUpdateStmt(ध्रुव VarForceInfo& varInfo) ध्रुव {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ध्रुव varp = varInfo.m_varVscp->varp();
        जइ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            प्रत्यागम createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_पङ्क्तिः* ध्रुव flp = varInfo.m_varVscp->fileline();
        प्रत्यागम नूतन AstAssign{flp, नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_ग्रन्थिः_Stmt* createForceRdUpdateStmtUnpacked(ध्रुव VarForceInfo& varInfo) ध्रुव {
        File_पङ्क्तिः* ध्रुव flp = varInfo.m_varVscp->fileline();
        AstVar* ध्रुव varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ध्रुव arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp = foreachUnpackedLeaf(
            dims, [&](ध्रुव std::vector<पूर्णाङ्क>& idx, पूर्णाङ्क /*flat*/) -> Ast_ग्रन्थिः_Stmt* {
                AstVarRef* ध्रुव baseRefp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp = नूतन AstOr{
                    flp, नूतन AstAnd{flp, enSelp, valSelp},
                    नूतन AstAnd{flp, नूतन AstNot{flp, enSelp->clone_वृक्षः_Pure(मिच्छ)}, baseSelp}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rdLhsSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                प्रत्यागम नूतन AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        प्रति (ध्रुव ForceInfo* ध्रुव finfop : forceInfosInIdOrder(varInfo)) {
            जइ (!finfop->m_isExternal) अनुवर्त;
            stmtsp->addNext(नूतन AstAssign{flp,
                                          नूतन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_वृक्षः_Pure(मिच्छ)});
        }
        प्रत्यागम stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ध्रुव auto it = m_varToId.find(vscp);
        जइ (it != m_varToId.end()) प्रत्यागम m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ध्रुव varp = info.m_varp;
        जइ (!varp->isForceable()) प्रत्यागम info;

        File_पङ्क्तिः* ध्रुव flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ध्रुव तार्किक helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        जइ (!helperVarsBuilt) {
            ध्रुव तार्किक unpacked = isUnpackedArrayDType(varp->dtypep());
            ध्रुव VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ग्रन्थिः_DType* ध्रुव enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = नूतन AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(सच्च);
            helperVars.m_enVarp
                = नूतन AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(सच्च);
            helperVars.m_valVarp
                = नूतन AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(सच्च);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        जइ (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } अण्णहा {
            info.m_forceRdVscp = नूतन AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = नूतन AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = नूतन AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        प्रत्यागम info;
    }

    शून्य markClockedWrite(AstVar* varp) { m_clockedWrites.निवेशय(varp); }
    तार्किक hasClockedWrite(AstVar* varp) ध्रुव { प्रत्यागम m_clockedWrites.गणनम्(varp); }

    तार्किक doingAssign() ध्रुव { प्रत्यागम m_doingAssign; }

    ध्रुव VarForceInfo* getVarInfo(AstVarScope* vscp) ध्रुव {
        ध्रुव auto it = m_varToId.find(vscp);
        प्रत्यागम it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ध्रुव AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        जइ (cache.empty()) {
            प्रति (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ध्रुव auto it = cache.find(varp);
        प्रत्यागम it != cache.end() ? it->second : nullptr;
    }
    शून्य addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsExprp,
                            AstAssignForce* forceStmtp, पूर्णाङ्क rangeLsb, पूर्णाङ्क rangeMsb, पूर्णाङ्क padLsb,
                            पूर्णाङ्क padMsb, तार्किक hasArraySel, तार्किक skipRdRefresh = मिच्छ) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ध्रुव पूर्णाङ्क forceId = info.m_forces.size();
        File_पङ्क्तिः* ध्रुव flp = varp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        जइ (!info.m_forceVecVscp) {
            AstCDType* ध्रुव forceVecDtypep = नूतन AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* ध्रुव forceVecVarp
                = नूतन AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(मिच्छ);
            forceVecVarp->isInternal(सच्च);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = नूतन AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        auto pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        जइ (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ध्रुव refp) {
                जइ (!refp->access().isReadOnly()) प्रत्यागम;
                AstVar* ध्रुव depVarp = refp->varp();
                जइ (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.प्रक्षिप_back(depVarp);
                }
            });
            प्रति (AstVar* ध्रुव depVarp : depVarps) m_rhsDepToForces[depVarp].प्रक्षिप_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    स्थिर शून्य collectArraySelInfo(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ArraySelInfo& info) {
        जइ (ध्रुव auto* ध्रुव selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = सच्च;
            collectArraySelInfo(selp->fromp(), info);
        } अण्णहा जइ (auto* ध्रुव selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.प्रक्षिप_back(selp);
        } अण्णहा जइ (ध्रुव auto* ध्रुव memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    स्थिर ArraySelInfo getArraySelInfo(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        प्रत्यागम info;
    }

    स्थिर std::vector<पूर्णाङ्क> arraySelDimSizes(ध्रुव ArraySelInfo& info) {
        std::vector<पूर्णाङ्क> dims;
        dims.reserve(info.m_sels.size());
        प्रति (AstArraySel* ध्रुव selp : info.m_sels) {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = selp->fromp()->dtypep()->skipRefp();
            ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.प्रक्षिप_back(arrayp->declRange().elements());
        }
        प्रत्यागम dims;
    }

    स्थिर पूर्णाङ्क flattenIndex(ध्रुव std::vector<पूर्णाङ्क>& indices, ध्रुव std::vector<पूर्णाङ्क>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        पूर्णाङ्क index = 0;
        पूर्णाङ्क stride = 1;
        प्रति (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        प्रत्यागम index;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* build_Flatten_Index_व्यञ्जकम्(File_पङ्क्तिः* flp, ध्रुव ArraySelInfo& info) {
        ध्रुव std::vector<पूर्णाङ्क> dimSizes = arraySelDimSizes(info);
        तार्किक allConst = सच्च;
        प्रति (AstArraySel* ध्रुव selp : info.m_sels) {
            जइ (!VN_IS(selp->bitp(), Const)) {
                allConst = मिच्छ;
                भङ्ग;
            }
        }
        जइ (allConst) {
            std::vector<पूर्णाङ्क> constIndices;
            constIndices.reserve(info.m_sels.size());
            प्रति (AstArraySel* ध्रुव selp : info.m_sels) {
                constIndices.प्रक्षिप_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            प्रत्यागम makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
        पूर्णाङ्क stride = 1;
        प्रति (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* termp = info.m_sels[i]->bitp()->clone_वृक्षः_Pure(मिच्छ);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            जइ (termp->width() < 32) termp = नूतन AstExtend{flp, termp, 32};
            जइ (stride != 1) termp = नूतन AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? नूतन AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        प्रत्यागम resultp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* build_Rhs_Data_व्यञ्जकम्(File_पङ्क्तिः* flp, ध्रुव ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        प्रत्यागम नूतन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    शून्य finalizeRhsVars() {
        प्रति (VarForceInfo& info : m_varInfos) {
            AstVar* ध्रुव varp = info.m_varp;
            जइ (info.m_forces.empty()) अनुवर्त;

            AstScope* ध्रुव scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_पङ्क्तिः* ध्रुव flp = varp->fileline();
            ध्रुव std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            प्रति (ForceInfo* ध्रुव finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ध्रुव rhsVarp = नूतन AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(सच्च);
                rhsVarp->sigPublic(सच्च);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = नूतन AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* ध्रुव rhsAssignp = नूतन AstAssign{
                    flp, नूतन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                जइ (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ध्रुव touchCallp = नूतन AstCMethodHard{
                        flp, नूतन AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_ग्रन्थिः_Stmt* ध्रुव touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ध्रुव alwaysp
                    = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_वृक्षः* ध्रुव senTreep
                    = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* ध्रुव activep = नूतन AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_वृक्षः_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            जइ (info.m_forceRdVscp) {
                जइ (!m_forceRdUpdateBuilt.निवेशय(info.m_varVscp).second) अनुवर्त;
                AstActive* ध्रुव activeInitp = नूतन AstActive{
                    flp, "force-init",
                    नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_वृक्षः_Storep(activeInitp->sentreep());
                Ast_ग्रन्थिः_Stmt* initStmtp = nullptr;
                जइ (AstUnpackArrayDType* ध्रुव arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ध्रुव std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ध्रुव पूर्णाङ्क innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](ध्रुव std::vector<पूर्णाङ्क>& idx, पूर्णाङ्क /*flat*/) -> Ast_ग्रन्थिः_Stmt* {
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = buildNestedArraySel(
                                flp, नूतन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            प्रत्यागम नूतन AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } अण्णहा {
                    initStmtp = नूतन AstAssign{
                        flp, नूतन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(नूतन AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                auto addSenItem = [&](AstVarScope* vscp) {
                    जइ (!vscp) प्रत्यागम;
                    AstSenItem* ध्रुव nextp = नूतन AstSenItem{
                        flp, VEdgeType::ET_CHANGED, नूतन AstVarRef{flp, vscp, VAccess::READ}};
                    जइ (itemsp) {
                        itemsp->addNext(nextp);
                    } अण्णहा {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* ध्रुव origSenRefp = नूतन AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ध्रुव origItemp
                    = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                जइ (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                प्रति (ForceInfo* ध्रुव finfop : forceps) {
                    जइ (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* ध्रुव activep
                    = नूतन AstActive{flp, "force-rd-update", नूतन Ast_Sen_वृक्षः{flp, itemsp}};
                activep->sen_वृक्षः_Storep(activep->sentreep());
                activep->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_ग्रन्थिः* createRhsUpdatesForWrite(File_पङ्क्तिः* flp, AstVar* writtenVarp) ध्रुव {
        जइ (!doingAssign()) प्रत्यागम nullptr;

        ध्रुव auto it = m_rhsDepToForces.find(writtenVarp);
        जइ (it == m_rhsDepToForces.end()) प्रत्यागम nullptr;

        Ast_ग्रन्थिः* headp = nullptr;
        Ast_ग्रन्थिः* tailp = nullptr;
        प्रति (ध्रुव ForceInfo* ध्रुव finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* ध्रुव updatep
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_वृक्षः_Pure(मिच्छ)};
            जइ (tailp) {
                tailp->addNextHere(updatep);
            } अण्णहा {
                headp = updatep;
            }
            tailp = updatep;
        }
        प्रत्यागम headp;
    }

    ध्रुव ForceInfo& getForceInfo(AstAssignForce* forceStmtp) ध्रुव {
        AstVarScope* ध्रुव vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ध्रुव VarForceInfo* ध्रुव varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        auto it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        प्रत्यागम it2->second;
    }

    स्थिर तार्किक selOverlapsAnyForce(ध्रुव VarForceInfo& varInfo, पूर्णाङ्क selLsb, पूर्णाङ्क selMsb) {
        प्रति (ध्रुव auto& pair : varInfo.m_forces) {
            जइ (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) प्रत्यागम सच्च;
        }
        प्रत्यागम मिच्छ;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceReadExpression(ध्रुव VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ध्रुव {
        File_पङ्क्तिः* ध्रुव flp = originalRefp->fileline();
        AstVarRef* ध्रुव refp = originalRefp->clone_वृक्षः_Pure(मिच्छ);
        refp->access(VAccess::READ);
        प्रत्यागम createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceReadIndexExpression(ध्रुव VarForceInfo& varInfo,
                                                Ast_ग्रन्थिः_व्यञ्जकम्* originalExprp,
                                                Ast_ग्रन्थिः_व्यञ्जकम्* indexExprp) ध्रुव {
        File_पङ्क्तिः* ध्रुव flp = originalExprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = originalExprp->clone_वृक्षः_Pure(मिच्छ);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        प्रत्यागम createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* rebuildSelPath(Ast_ग्रन्थिः_व्यञ्जकम्* pathp, Ast_ग्रन्थिः_व्यञ्जकम्* baseExprp) {
        जइ (ध्रुव AstSel* ध्रुव selp = VN_CAST(pathp, Sel)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* ध्रुव outp
                = नूतन AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            प्रत्यागम outp;
        }
        प्रत्यागम baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
स्थिर शून्य splitDeassign(AstDeassign* nodep) {
    AstConcat* ध्रुव concatp = VN_CAST(nodep->lhsp(), Concat);
    जइ (!concatp) प्रत्यागम;

    File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
    AstDeassign* ध्रुव newLp = नूतन AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* ध्रुव newRp = नूतन AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    VL_DO_DANGLING(conp->delete_वृक्षः(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

वर्ग ForceDiscoveryVisitor final : सार्वजनिक VNVisitorConst {
    ForceState& m_state;
    तार्किक m_inClockedActive = मिच्छ;

    शून्य buildForceableUnpackedArray(AstVarScope* ध्रुव nodep,
                                     AstUnpackArrayDType* ध्रुव arrDtypep) {
        AstVar* ध्रुव varp = nodep->varp();
        ध्रुव std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव leafDtypep = dims.back()->subDTypep()->skipRefp();
        ध्रुव AstBasicDType* ध्रुव innerBasicp = leafDtypep->basicp();
        जइ (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            प्रत्यागम;
        }

        File_पङ्क्तिः* ध्रुव flp = varp->fileline();
        ध्रुव पूर्णाङ्क innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ध्रुव enVscp = info.m_forceEnVscp;
        AstVarScope* ध्रुव valVscp = info.m_forceValVscp;

        AstSenItem* ध्रुव itemsp = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  नूतन AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* ध्रुव activep = नूतन AstActive{flp, "force-update", नूतन Ast_Sen_वृक्षः{flp, itemsp}};
        activep->sen_वृक्षः_Storep(activep->sentreep());

        Ast_ग्रन्थिः_Stmt* ध्रुव alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](ध्रुव std::vector<पूर्णाङ्क>& idx, पूर्णाङ्क flat) -> Ast_ग्रन्थिः_Stmt* {
                AstVarRef* ध्रुव origRefp = नूतन AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव forceExprp = नूतन AstOr{
                    flp, नूतन AstAnd{flp, enSelp, valSelp},
                    नूतन AstAnd{flp, नूतन AstNot{flp, enSelp->clone_वृक्षः_Pure(मिच्छ)}, origSelp}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* ध्रुव forceAssignp = नूतन AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(सच्च);

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsClonep = forceExprp->clone_वृक्षः_Pure(मिच्छ);
                rhsClonep->foreach([varp](AstVarRef* ध्रुव r) {
                    जइ (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/सच्च, /*skipRdRefresh=*/सच्च);
                प्रत्यागम forceAssignp;
            });
        activep->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    शून्य visit(AstAssignForce* nodep) override {
        जइ (nodep->user2()) प्रत्यागम;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ध्रुव lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ध्रुव forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, सच्च);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsExprp = nodep->rhsp()->clone_वृक्षः_Pure(मिच्छ);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    शून्य visit(AstAssign* nodep) override {
        जइ (m_state.doingAssign() && m_inClockedActive) {
            जइ (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarScope* nodep) override {
        जइ (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            जइ (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                प्रत्यागम;
            }

            जइ (AstUnpackArrayDType* ध्रुव arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                प्रत्यागम;
            }

            ध्रुव AstBasicDType* ध्रुव bdtypep = nodep->varp()->basicp();
            जइ (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* ध्रुव varp = nodep->varp();
            File_पङ्क्तिः* ध्रुव flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* ध्रुव enVscp = info.m_forceEnVscp;
            AstVarScope* ध्रुव valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* ध्रुव itemsp = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      नूतन AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* ध्रुव activep
                = नूतन AstActive{flp, "force-update", नूतन Ast_Sen_वृक्षः{flp, itemsp}};
            activep->sen_वृक्षः_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* ध्रुव enRefp = नूतन AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* ध्रुव valRefp = नूतन AstVarRef{flp, valVscp, VAccess::READ};
            ध्रुव AstBasicDType* ध्रुव basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstOr{
                            flp, नूतन AstAnd{flp, enRefp, valRefp},
                            नूतन AstAnd{flp, नूतन AstNot{flp, enRefp->clone_वृक्षः_Pure(मिच्छ)}, refp}})
                      : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* ध्रुव forceAssignp
                = नूतन AstAssignForce{flp, नूतन AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(सच्च);
            activep->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsClonep = forceExprp->clone_वृक्षः_Pure(मिच्छ);
            rhsClonep->foreach([varp](AstVarRef* ध्रुव refp) {
                जइ (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ध्रुव तार्किक bitwiseVar = ForceState::isBitwiseDType(varp);
            ध्रुव पूर्णाङ्क padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            पूर्णाङ्क rangeLsb = 0;
            पूर्णाङ्क rangeMsb = padMsb;
            जइ (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, मिच्छ);
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

वर्ग ForceConvertVisitor final : सार्वजनिक VNVisitor {
    ForceState& m_state;

    शून्य visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        AstVarRef* ध्रुव lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ध्रुव forcedVarp = lhsVarRefp->varp();

        ध्रुव ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* ध्रुव rhsAssignp
            = नूतन AstAssign{flp, नूतन AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_वृक्षः_Pure(मिच्छ)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        ध्रुव तार्किक bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        जइ (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ग्रन्थिः_व्यञ्जकम्* baseExprp = nodep->rhsp()->clone_वृक्षः_Pure(मिच्छ);
            baseExprp->foreach(
                [](AstVarRef* ध्रुव refp) { ForceState::markNonReplaceable(refp); });
            जइ (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            जइ (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ध्रुव maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valReadp
                    = नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valWritep
                    = नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notMaskp = नूतन AstNot{flp, maskConstp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maskedOldp = नूतन AstAnd{flp, valReadp, notMaskp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newValp = नूतन AstOr{flp, maskedOldp, baseExprp};
                valAssignp = नूतन AstAssign{flp, valWritep, newValp};

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enReadp
                    = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enWritep
                    = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newEnp
                    = नूतन AstOr{flp, enReadp, maskConstp->clone_वृक्षः_Pure(मिच्छ)};
                enAssignp = नूतन AstAssign{flp, enWritep, newEnp};
            } अण्णहा {
                AstConst* ध्रुव oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsValp = ForceState::cast_To_ग्रन्थिः_DType(baseExprp, forcedVarp);
                valAssignp = नूतन AstAssign{
                    flp, नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = नूतन AstAssign{
                    flp, नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ध्रुव AstSel* ध्रुव selLhsp = VN_CAST(lhsp, Sel);
        ध्रुव तार्किक arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsDatap = ForceState::build_Rhs_Data_व्यञ्जकम्(flp, info);
        AstCExpr* ध्रुव rhsAddrp = नूतन AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* ध्रुव addForceCallp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        जइ (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = addForceCallp->makeStmt();

        Ast_ग्रन्थिः* tailp = rhsAssignp;
        जइ (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        जइ (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        जइ (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        AstVarRef* ध्रुव lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ध्रुव releasedVarp = lhsVarRefp->varp();

        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        जइ (!varInfo) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        ध्रुव ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, मिच्छ);

        ध्रुव AstSel* ध्रुव selLhsp = VN_CAST(lhsp, Sel);
        ध्रुव तार्किक arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* ध्रुव releaseCallp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        जइ (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ग्रन्थिः_Stmt* ध्रुव releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        जइ (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enWritep
                = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            जइ (ForceState::isBitwiseDType(releasedVarp)) {
                ध्रुव पूर्णाङ्क varWidth = releasedVarp->width();
                जइ (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = नूतन AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } अण्णहा {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enReadp
                        = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ध्रुव maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newEnp
                        = नूतन AstAnd{flp, enReadp, नूतन AstNot{flp, maskConst}};
                    clearEnp = नूतन AstAssign{flp, enWritep, newEnp};
                }
            } अण्णहा {
                clearEnp = नूतन AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ध्रुव AstSel* ध्रुव selp = VN_CAST(lhsp, Sel);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव basep = selp ? selp->fromp() : lhsp;

        Ast_ग्रन्थिः* stmtListp = releasep;
        जइ (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ध्रुव तार्किक fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        जइ (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ध्रुव तार्किक hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ग्रन्थिः_व्यञ्जकम्* forceReadp = nullptr;
            जइ (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } अण्णहा जइ (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_व्यञ्जकम्(flp, rangeInfo.m_arrayInfo));
                जइ (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } अण्णहा {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* ध्रुव assignp = नूतन AstAssign{flp, lhsp->clone_वृक्षः_Pure(मिच्छ), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        जइ (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

वर्ग ForceReplaceVisitor final : सार्वजनिक VNVisitor {
    ध्रुव ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ग्रन्थिः_Stmt* m_stmtp = nullptr;
    तार्किक m_inLogic = मिच्छ;

    शून्य iterateLogic(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = सच्च;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        जइ (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp()->baseFromp(सच्च), VarRef)) {
            जइ (Ast_ग्रन्थिः* ध्रुव updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->addNextHere(updatep);
            }
        }
    }
    शून्य visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateAndNextNull(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    शून्य visit(AstDeassign*) override {}
    शून्य visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstAlways* nodep) override {
        जइ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        iterateLogic(nodep);
    }
    शून्य visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef);
        जइ (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_ग्रन्थिः*>(nodep));
            प्रत्यागम;
        }

        AstVar* ध्रुव varp = refp->varp();
        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo = m_state.getVarInfo(refp->varScopep());
        जइ (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_ग्रन्थिः*>(nodep));
            प्रत्यागम;
        }

        जइ (ध्रुव AstConst* ध्रुव lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ध्रुव पूर्णाङ्क selLsb = lsbConstp->toSInt();
            ध्रुव पूर्णाङ्क selMsb = selLsb + nodep->width() - 1;
            जइ (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_ग्रन्थिः*>(nodep));
                प्रत्यागम;
            }
        }

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ध्रुव refClonep = refp->clone_वृक्षः_Pure(मिच्छ);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_वृक्षः_Pure(मिच्छ));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstArraySel* nodep) override {
        जइ (ध्रुव AstArraySel* ध्रुव backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            जइ (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                प्रत्यागम;
            }
        }

        Ast_ग्रन्थिः* ध्रुव basep = nodep->baseFromp(सच्च);
        AstVarRef* ध्रुव baseRefp = VN_CAST(basep, VarRef);
        जइ (!baseRefp) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        AstVar* ध्रुव varp = baseRefp->varp();
        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        जइ (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            प्रत्यागम;
        }

        जइ (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        जइ (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            प्रत्यागम;
        }
        ध्रुव ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        प्रति (AstArraySel* ध्रुव selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexExprp
            = ForceState::build_Flatten_Index_व्यञ्जकम्(nodep->fileline(), arrayInfo);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        जइ (m_state.isNotReplaceable(nodep)) प्रत्यागम;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        जइ (ध्रुव AstArraySel* ध्रुव backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            जइ (backSelp->fromp() == nodep) प्रत्यागम;
        }

        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo = m_state.getVarInfo(nodep->varScopep());
        जइ (!varInfo) प्रत्यागम;

        जइ (varInfo->m_forceRdVscp) {
            जइ (nodep->access().isRW()) {
                जइ (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                प्रत्यागम;
            }
            जइ (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
            जइ (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            प्रत्यागम;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        जइ (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            प्रत्यागम;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        जइ (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } अण्णहा जइ (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_AS(nodep, ग्रन्थिः_व्यञ्जकम्);
            Ast_ग्रन्थिः* ध्रुव basep = exprp->baseFromp(सच्च);
            AstVarRef* ध्रुव baseRefp = VN_CAST(basep, VarRef);
            जइ (baseRefp) {
                AstVar* ध्रुव varp = baseRefp->varp();
                जइ (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ध्रुव ForceState::VarForceInfo* ध्रुव varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    जइ (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        ध्रुव पूर्णाङ्क forcePathIndex = varInfo->findForcePathIndex(exprp);
                        जइ (forcePathIndex >= 0) {
                            जइ (!baseRefp->access().isReadOnly()) प्रत्यागम;
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                            प्रत्यागम;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    explicit ForceReplaceVisitor(AstNetlist* nodep, ध्रुव ForceState& state)
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

स्थिर शून्य forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    जइ (!v3Global.hasForceableSignals()) प्रत्यागम;
    ForceState state{मिच्छ, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्षः("force", 0, dump_वृक्षः_Either_Level() >= 3);
}

स्थिर शून्य assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    जइ (!v3Global.hasAssignDeassign()) प्रत्यागम;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.प्रक्षिप_back(deassignp); });
    प्रति (AstDeassign* ध्रुव deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_ग्रन्थिः_Stmt* nodep) {
        जइ (AstAssignCont* ध्रुव assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.प्रक्षिप_back(assignContp);
        } अण्णहा जइ (AstDeassign* ध्रुव deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.प्रक्षिप_back(deassignp);
        }
    });

    प्रति (AstAssignCont* ध्रुव assignp : assignContps) {
        assignp->replaceWith(नूतन AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_वृक्षः(), assignp);
    }
    प्रति (AstDeassign* ध्रुव deassignp : deassignps) {
        deassignp->replaceWith(
            नूतन AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_वृक्षः_Pure(सच्च)});
        VL_DO_DANGLING(deassignp->delete_वृक्षः(), deassignp);
    }
    ForceState state{सच्च, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्षः("assign-deassign", 0, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3Force::forceAndAssignAll(AstNetlist* nodep) {
    ध्रुव VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
