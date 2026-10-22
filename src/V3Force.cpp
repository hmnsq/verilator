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
        पूर्णांक m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        पूर्णांक m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        पूर्णांक m_padLsb = 0;  // Bit positions for RHS padding
        पूर्णांक m_padMsb = 0;  // Bit positions for RHS padding
    };

    संरचना ForceInfo final : ForceRange {
        // MEMBERS
        पूर्णांक m_forceId = 0;  // Unique (per signal) variable of this force assignment
        तार्किक m_hasArraySel = असत्य;  // If this has an array select on LHS
        तार्किक m_isExternal = असत्य;  // Synthetic force for the public force controls
        तार्किक m_skipRdRefresh = असत्य;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_गाठ_अभिव्यक्ती* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = पूर्वनिर्धारित;
        ForceInfo(पूर्णांक rangeLsb, पूर्णांक rangeMsb, पूर्णांक padLsb, पूर्णांक padMsb, पूर्णांक forceId,
                  तार्किक hasArraySel, AstVarScope* rhsVarVscp, Ast_गाठ_अभिव्यक्ती* rhsExprp)
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
        std::unordered_map<string, पूर्णांक> m_forcePathToIndex;
        पूर्णांक m_nextForcePathIndex = 1;  // Start at 1 so 0 can be the base path (whole signal)

    खाजगी:
        स्थिर शून्य build_Force_Path_किल्ली_Recurse(Ast_गाठ_अभिव्यक्ती* nodep, string& out) {
            जर (VN_IS(nodep, VarRef)) परत;
            जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_किल्ली_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                परत;
            }
            जर (AstStructSel* स्थिरांक selp = VN_CAST(nodep, StructSel)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = selp->fromp();
                build_Force_Path_किल्ली_Recurse(fromp, out);
                out += "|T" + selp->name();
                परत;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        स्थिर string force_Path_किल्ली(Ast_गाठ_अभिव्यक्ती* nodep) {
            string out;
            build_Force_Path_किल्ली_Recurse(nodep, out);
            परत out;
        }

    सार्वजनिक:
        पूर्णांक getOrCreateForcePathIndex(Ast_गाठ_अभिव्यक्ती* nodep) {
            स्थिरांक स्वतः pair
                = m_forcePathToIndex.emplace(force_Path_किल्ली(nodep), m_nextForcePathIndex);
            जर (pair.second) ++m_nextForcePathIndex;
            परत pair.first->second;
        }

        पूर्णांक findForcePathIndex(Ast_गाठ_अभिव्यक्ती* nodep) स्थिरांक {
            स्थिरांक स्वतः it = m_forcePathToIndex.find(force_Path_किल्ली(nodep));
            परत it != m_forcePathToIndex.end() ? it->second : -1;
        }
    };

    संरचना ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    संरचना ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        तार्किक m_hasBitSel = असत्य;  // Bit-select was found along the path too
    };

    संरचना ForceRangeInfo final : ForceRange {
        तार्किक m_hasArraySel = असत्य;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

खाजगी:
    using ScopeVarCache = std::unordered_map<स्थिरांक AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    स्थिरांक VNUser1InUse m_user1InUse;
    स्थिरांक VNUser2InUse m_user2InUse;

सार्वजनिक:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<स्थिरांक AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<स्थिरांक AstVarScope*>;

खाजगी:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, पूर्णांक> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    तार्किक m_doingAssign = असत्य;  // If true, we're processing procedural continuous assign
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
    स्थिर AstConst* makeZeroConst(Ast_गाठ* nodep, पूर्णांक width) {
        V3_संख्या zero{nodep, width};
        zero.setAllBits0();
        परत नवीन AstConst{nodep->fileline(), zero};
    }

    स्थिर AstConst* makeConst32(File_ओळ* flp, पूर्णांक मूल्य) {
        परत नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, static_cast<uint32_t>(मूल्य)};
    }

    स्थिर AstConst* makeRangeMaskConst(Ast_गाठ* nodep, पूर्णांक width, पूर्णांक lsb, पूर्णांक msb) {
        V3_संख्या mask{nodep, width};
        mask.setAllBits0();
        साठी (पूर्णांक bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        परत नवीन AstConst{nodep->fileline(), mask};
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* zeroPadToBaseWidth(Ast_गाठ_अभिव्यक्ती* exprp, पूर्णांक baseWidth, पूर्णांक padLsb,
                                           पूर्णांक padMsb) {
        जर (baseWidth <= 0) परत exprp;
        स्थिरांक पूर्णांक खालचा_Pad = padLsb;
        स्थिरांक पूर्णांक वरचा_Pad = baseWidth - (padMsb + 1);
        जर (खालचा_Pad > 0) {
            exprp = नवीन AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, खालचा_Pad)};
        }
        जर (वरचा_Pad > 0) {
            exprp = नवीन AstConcat{exprp->fileline(), makeZeroConst(exprp, वरचा_Pad), exprp};
        }
        परत exprp;
    }

    स्थिर तार्किक isUnpackedArrayDType(स्थिरांक Ast_गाठ_DType* dtypep) {
        परत VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    स्थिर तार्किक isBitwiseDType(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक AstBasicDType* स्थिरांक basicp = nodep->dtypep()->skipRefp()->basicp();
        परत basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* cast_To_गाठ_DType(Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ* dtypeFromp) {
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = dtypeFromp->dtypep()->skipRefp();
        स्थिरांक AstBasicDType* स्थिरांक basicp = dtypep->basicp();
        जर (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            परत exprp;
        }
        परत नवीन AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    स्थिर शून्य markNonReplaceable(AstVarRef* स्थिरांक nodep) { nodep->user1SetOnce(); }
    तार्किक isNotReplaceable(स्थिरांक AstVarRef* स्थिरांक nodep) स्थिरांक {
        परत nodep->user1() || m_permanentlyProtected.मोजणी(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    शून्य markPermanentlyProtected(AstVarRef* स्थिरांक nodep) स्थिरांक {
        markNonReplaceable(nodep);
        m_permanentlyProtected.घाला(nodep);
    }

    स्थिर std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        साठी (स्वतः& it : info.m_forces) forceps.ढकला_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](स्थिरांक ForceInfo* ap, स्थिरांक ForceInfo* bp) {
            परत ap->m_forceId < bp->m_forceId;
        });
        परत forceps;
    }

    स्थिर std::vector<स्थिरांक ForceInfo*> forceInfosInIdOrder(स्थिरांक VarForceInfo& info) {
        std::vector<स्थिरांक ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        साठी (स्थिरांक स्वतः& it : info.m_forces) forceps.ढकला_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](स्थिरांक ForceInfo* ap, स्थिरांक ForceInfo* bp) {
            परत ap->m_forceId < bp->m_forceId;
        });
        परत forceps;
    }

    स्थिर तार्किक isOpaquePathSelector(स्थिरांक Ast_गाठ* nodep) {
        परत VN_IS(nodep, Sel) || VN_IS(nodep, गाठ_Sel) || VN_IS(nodep, StructSel);
    }

    स्थिर तार्किक isOutermostOpaquePathSelector(स्थिरांक Ast_गाठ* nodep) {
        जर (!isOpaquePathSelector(nodep)) परत असत्य;
        स्थिरांक Ast_गाठ* स्थिरांक backp = nodep->backp();
        परत !backp || !isOpaquePathSelector(backp);
    }

    स्थिर AstVarRef* getOneVarRef(Ast_गाठ_अभिव्यक्ती* forceStmtp) {
        Ast_गाठ* स्थिरांक basep = forceStmtp->baseFromp(सत्य);
        जर (AstSampled* sampledp = VN_CAST(basep, Sampled))
            जर (Ast_गाठ_अभिव्यक्ती* exprp = VN_CAST(sampledp->exprp(), गाठ_अभिव्यक्ती))
                परत getOneVarRef(exprp);
        AstVarRef* स्थिरांक varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        परत varRefp;
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* buildNestedArraySel(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* fromp,
                                            स्थिरांक std::vector<पूर्णांक>& indicies) {
        Ast_गाठ_अभिव्यक्ती* curp = fromp;
        साठी (स्थिरांक पूर्णांक idx : indicies) curp = नवीन AstArraySel{flp, curp, idx};
        परत curp;
    }

    ढाँचा <typename Fn>
    स्थिर Ast_गाठ_Stmt* foreachUnpackedLeaf(स्थिरांक std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_गाठ_Stmt* headp = nullptr;
        Ast_गाठ_Stmt* tailp = nullptr;
        जर (dims.empty()) परत nullptr;
        पूर्णांक total = 1;
        साठी (स्थिरांक AstUnpackArrayDType* स्थिरांक d : dims) total *= d->elementsConst();
        जर (total <= 0) परत nullptr;
        std::vector<पूर्णांक> idx(dims.size(), 0);
        साठी (पूर्णांक flat = 0; flat < total; ++flat) {
            Ast_गाठ_Stmt* स्थिरांक stmtp = buildLeaf(idx, flat);
            जर (!headp) {
                headp = stmtp;
            } अन्यथा {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            साठी (पूर्णांक d = static_cast<पूर्णांक>(dims.size()) - 1; d >= 0; --d) {
                जर (++idx[d] < dims[d]->elementsConst()) थांबा;
                idx[d] = 0;
            }
        }
        परत headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_गाठ_अभिव्यक्ती* lhsp, AstVar* varp,
                                     तार्किक requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        जर (स्थिरांक AstSel* स्थिरांक outerSelp = VN_CAST(lhsp, Sel)) {
            पूर्णांक totalLsb = 0;
            साठी (Ast_गाठ_अभिव्यक्ती* curp = lhsp; स्थिरांक AstSel* स्थिरांक selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                जर (requireConstRangeSelect) {
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
        जर (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<पूर्णांक> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            साठी (AstArraySel* स्थिरांक selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.ढकला_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } अन्यथा जर (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        जर (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            स्थिरांक पूर्णांक index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        परत info;
    }

    Ast_गाठ_अभिव्यक्ती* add_Rhs_मूल्य_Reads(स्थिरांक VarForceInfo& varInfo, Ast_गाठ_अभिव्यक्ती* exprp) स्थिरांक {
        जर (!doingAssign()) परत exprp;

        स्थिरांक std::vector<स्थिरांक ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        जर (forceps.empty()) परत exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* स्थिरांक cexprp = नवीन AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        साठी (स्थिरांक ForceInfo* स्थिरांक finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* स्थिरांक refp
                = नवीन AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        परत cexprp;
    }

    Ast_गाठ_अभिव्यक्ती* createForceReadCall(स्थिरांक VarForceInfo& varInfo, File_ओळ* flp, VCMethod method,
                                     Ast_गाठ_अभिव्यक्ती* originalExprp, Ast_गाठ* dtypeFromp,
                                     Ast_गाठ_अभिव्यक्ती* indexExprp) स्थिरांक {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_गाठ_अभिव्यक्ती* स्थिरांक origValp
            = add_Rhs_मूल्य_Reads(varInfo, cast_To_गाठ_DType(originalExprp, dtypeFromp));

        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        जर (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        परत callp;
    }

    Ast_गाठ_अभिव्यक्ती* createForceRdExpression(स्थिरांक VarForceInfo& varInfo, File_ओळ* flp) स्थिरांक {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* स्थिरांक varp = varInfo.m_varVscp->varp();
        जर (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            जर (doingAssign()) { परत नवीन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सत्य);
            AstVarRef* स्थिरांक baseRefp = नवीन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            परत createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* स्थिरांक baseRefp = नवीन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक enRefp = नवीन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valRefp = नवीन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        जर (isBitwiseDType(varp)) {
            परत नवीन AstOr{
                flp, नवीन AstAnd{flp, enRefp, valRefp},
                नवीन AstAnd{flp, नवीन AstNot{flp, enRefp->clone_वृक्ष_Pure(असत्य)}, baseRefp}};
        }
        परत नवीन AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_गाठ_Stmt* createForceRdUpdateStmt(स्थिरांक VarForceInfo& varInfo) स्थिरांक {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* स्थिरांक varp = varInfo.m_varVscp->varp();
        जर (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            परत createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_ओळ* स्थिरांक flp = varInfo.m_varVscp->fileline();
        परत नवीन AstAssign{flp, नवीन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_गाठ_Stmt* createForceRdUpdateStmtUnpacked(स्थिरांक VarForceInfo& varInfo) स्थिरांक {
        File_ओळ* स्थिरांक flp = varInfo.m_varVscp->fileline();
        AstVar* स्थिरांक varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* स्थिरांक arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिरांक std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_गाठ_Stmt* स्थिरांक stmtsp = foreachUnpackedLeaf(
            dims, [&](स्थिरांक std::vector<पूर्णांक>& idx, पूर्णांक /*flat*/) -> Ast_गाठ_Stmt* {
                AstVarRef* स्थिरांक baseRefp = नवीन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक enSelp = buildNestedArraySel(
                    flp, नवीन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक valSelp = buildNestedArraySel(
                    flp, नवीन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक readExprp = नवीन AstOr{
                    flp, नवीन AstAnd{flp, enSelp, valSelp},
                    नवीन AstAnd{flp, नवीन AstNot{flp, enSelp->clone_वृक्ष_Pure(असत्य)}, baseSelp}};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक rdLhsSelp = buildNestedArraySel(
                    flp, नवीन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                परत नवीन AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        साठी (स्थिरांक ForceInfo* स्थिरांक finfop : forceInfosInIdOrder(varInfo)) {
            जर (!finfop->m_isExternal) सुरूठेवा;
            stmtsp->addNext(नवीन AstAssign{flp,
                                          नवीन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_वृक्ष_Pure(असत्य)});
        }
        परत stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        स्थिरांक स्वतः it = m_varToId.find(vscp);
        जर (it != m_varToId.end()) परत m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* स्थिरांक varp = info.m_varp;
        जर (!varp->isForceable()) परत info;

        File_ओळ* स्थिरांक flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        स्थिरांक तार्किक helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        जर (!helperVarsBuilt) {
            स्थिरांक तार्किक unpacked = isUnpackedArrayDType(varp->dtypep());
            स्थिरांक VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_गाठ_DType* स्थिरांक enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = नवीन AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(सत्य);
            helperVars.m_enVarp
                = नवीन AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(सत्य);
            helperVars.m_valVarp
                = नवीन AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(सत्य);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        जर (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } अन्यथा {
            info.m_forceRdVscp = नवीन AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = नवीन AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = नवीन AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        परत info;
    }

    शून्य markClockedWrite(AstVar* varp) { m_clockedWrites.घाला(varp); }
    तार्किक hasClockedWrite(AstVar* varp) स्थिरांक { परत m_clockedWrites.मोजणी(varp); }

    तार्किक doingAssign() स्थिरांक { परत m_doingAssign; }

    स्थिरांक VarForceInfo* getVarInfo(AstVarScope* vscp) स्थिरांक {
        स्थिरांक स्वतः it = m_varToId.find(vscp);
        परत it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, स्थिरांक AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        जर (cache.empty()) {
            साठी (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        स्थिरांक स्वतः it = cache.find(varp);
        परत it != cache.end() ? it->second : nullptr;
    }
    शून्य addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_गाठ_अभिव्यक्ती* rhsExprp,
                            AstAssignForce* forceStmtp, पूर्णांक rangeLsb, पूर्णांक rangeMsb, पूर्णांक padLsb,
                            पूर्णांक padMsb, तार्किक hasArraySel, तार्किक skipRdRefresh = असत्य) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        स्थिरांक पूर्णांक forceId = info.m_forces.size();
        File_ओळ* स्थिरांक flp = varp->fileline();
        AstScope* स्थिरांक scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        जर (!info.m_forceVecVscp) {
            AstCDType* स्थिरांक forceVecDtypep = नवीन AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* स्थिरांक forceVecVarp
                = नवीन AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(असत्य);
            forceVecVarp->isInternal(सत्य);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = नवीन AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        स्वतः pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        जर (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* स्थिरांक refp) {
                जर (!refp->access().isReadOnly()) परत;
                AstVar* स्थिरांक depVarp = refp->varp();
                जर (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.ढकला_back(depVarp);
                }
            });
            साठी (AstVar* स्थिरांक depVarp : depVarps) m_rhsDepToForces[depVarp].ढकला_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    स्थिर शून्य collectArraySelInfo(Ast_गाठ_अभिव्यक्ती* exprp, ArraySelInfo& info) {
        जर (स्थिरांक स्वतः* स्थिरांक selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = सत्य;
            collectArraySelInfo(selp->fromp(), info);
        } अन्यथा जर (स्वतः* स्थिरांक selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.ढकला_back(selp);
        } अन्यथा जर (स्थिरांक स्वतः* स्थिरांक memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    स्थिर ArraySelInfo getArraySelInfo(Ast_गाठ_अभिव्यक्ती* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        परत info;
    }

    स्थिर std::vector<पूर्णांक> arraySelDimSizes(स्थिरांक ArraySelInfo& info) {
        std::vector<पूर्णांक> dims;
        dims.reserve(info.m_sels.size());
        साठी (AstArraySel* स्थिरांक selp : info.m_sels) {
            Ast_गाठ_DType* स्थिरांक dtypep = selp->fromp()->dtypep()->skipRefp();
            स्थिरांक AstUnpackArrayDType* स्थिरांक arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.ढकला_back(arrayp->declRange().elements());
        }
        परत dims;
    }

    स्थिर पूर्णांक flattenIndex(स्थिरांक std::vector<पूर्णांक>& indices, स्थिरांक std::vector<पूर्णांक>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        पूर्णांक index = 0;
        पूर्णांक stride = 1;
        साठी (पूर्णांक i = static_cast<पूर्णांक>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        परत index;
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* build_Flatten_Index_अभिव्यक्ती(File_ओळ* flp, स्थिरांक ArraySelInfo& info) {
        स्थिरांक std::vector<पूर्णांक> dimSizes = arraySelDimSizes(info);
        तार्किक allConst = सत्य;
        साठी (AstArraySel* स्थिरांक selp : info.m_sels) {
            जर (!VN_IS(selp->bitp(), Const)) {
                allConst = असत्य;
                थांबा;
            }
        }
        जर (allConst) {
            std::vector<पूर्णांक> constIndices;
            constIndices.reserve(info.m_sels.size());
            साठी (AstArraySel* स्थिरांक selp : info.m_sels) {
                constIndices.ढकला_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            परत makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
        पूर्णांक stride = 1;
        साठी (पूर्णांक i = static_cast<पूर्णांक>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_गाठ_अभिव्यक्ती* termp = info.m_sels[i]->bitp()->clone_वृक्ष_Pure(असत्य);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            जर (termp->width() < 32) termp = नवीन AstExtend{flp, termp, 32};
            जर (stride != 1) termp = नवीन AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? नवीन AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        परत resultp;
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* build_Rhs_Data_अभिव्यक्ती(File_ओळ* flp, स्थिरांक ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        परत नवीन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    शून्य finalizeRhsVars() {
        साठी (VarForceInfo& info : m_varInfos) {
            AstVar* स्थिरांक varp = info.m_varp;
            जर (info.m_forces.empty()) सुरूठेवा;

            AstScope* स्थिरांक scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_ओळ* स्थिरांक flp = varp->fileline();
            स्थिरांक std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            साठी (ForceInfo* स्थिरांक finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* स्थिरांक rhsVarp = नवीन AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(सत्य);
                rhsVarp->sigPublic(सत्य);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = नवीन AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* स्थिरांक rhsAssignp = नवीन AstAssign{
                    flp, नवीन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                जर (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* स्थिरांक touchCallp = नवीन AstCMethodHard{
                        flp, नवीन AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_गाठ_Stmt* स्थिरांक touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* स्थिरांक alwaysp
                    = नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_वृक्ष* स्थिरांक senTreep
                    = नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* स्थिरांक activep = नवीन AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_वृक्ष_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            जर (info.m_forceRdVscp) {
                जर (!m_forceRdUpdateBuilt.घाला(info.m_varVscp).second) सुरूठेवा;
                AstActive* स्थिरांक activeInitp = नवीन AstActive{
                    flp, "force-init",
                    नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_वृक्ष_Storep(activeInitp->sentreep());
                Ast_गाठ_Stmt* initStmtp = nullptr;
                जर (AstUnpackArrayDType* स्थिरांक arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    स्थिरांक std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    स्थिरांक पूर्णांक innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](स्थिरांक std::vector<पूर्णांक>& idx, पूर्णांक /*flat*/) -> Ast_गाठ_Stmt* {
                            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = buildNestedArraySel(
                                flp, नवीन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            परत नवीन AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } अन्यथा {
                    initStmtp = नवीन AstAssign{
                        flp, नवीन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(नवीन AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                स्वतः addSenItem = [&](AstVarScope* vscp) {
                    जर (!vscp) परत;
                    AstSenItem* स्थिरांक nextp = नवीन AstSenItem{
                        flp, VEdgeType::ET_CHANGED, नवीन AstVarRef{flp, vscp, VAccess::READ}};
                    जर (itemsp) {
                        itemsp->addNext(nextp);
                    } अन्यथा {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* स्थिरांक origSenRefp = नवीन AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* स्थिरांक origItemp
                    = नवीन AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                जर (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                साठी (ForceInfo* स्थिरांक finfop : forceps) {
                    जर (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* स्थिरांक activep
                    = नवीन AstActive{flp, "force-rd-update", नवीन Ast_Sen_वृक्ष{flp, itemsp}};
                activep->sen_वृक्ष_Storep(activep->sentreep());
                activep->addStmtsp(नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_गाठ* createRhsUpdatesForWrite(File_ओळ* flp, AstVar* writtenVarp) स्थिरांक {
        जर (!doingAssign()) परत nullptr;

        स्थिरांक स्वतः it = m_rhsDepToForces.find(writtenVarp);
        जर (it == m_rhsDepToForces.end()) परत nullptr;

        Ast_गाठ* headp = nullptr;
        Ast_गाठ* tailp = nullptr;
        साठी (स्थिरांक ForceInfo* स्थिरांक finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* स्थिरांक updatep
                = नवीन AstAssign{flp, नवीन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_वृक्ष_Pure(असत्य)};
            जर (tailp) {
                tailp->addNextHere(updatep);
            } अन्यथा {
                headp = updatep;
            }
            tailp = updatep;
        }
        परत headp;
    }

    स्थिरांक ForceInfo& getForceInfo(AstAssignForce* forceStmtp) स्थिरांक {
        AstVarScope* स्थिरांक vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        स्थिरांक VarForceInfo* स्थिरांक varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        स्वतः it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        परत it2->second;
    }

    स्थिर तार्किक selOverlapsAnyForce(स्थिरांक VarForceInfo& varInfo, पूर्णांक selLsb, पूर्णांक selMsb) {
        साठी (स्थिरांक स्वतः& pair : varInfo.m_forces) {
            जर (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) परत सत्य;
        }
        परत असत्य;
    }

    Ast_गाठ_अभिव्यक्ती* createForceReadExpression(स्थिरांक VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) स्थिरांक {
        File_ओळ* स्थिरांक flp = originalRefp->fileline();
        AstVarRef* स्थिरांक refp = originalRefp->clone_वृक्ष_Pure(असत्य);
        refp->access(VAccess::READ);
        परत createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_गाठ_अभिव्यक्ती* createForceReadIndexExpression(स्थिरांक VarForceInfo& varInfo,
                                                Ast_गाठ_अभिव्यक्ती* originalExprp,
                                                Ast_गाठ_अभिव्यक्ती* indexExprp) स्थिरांक {
        File_ओळ* स्थिरांक flp = originalExprp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = originalExprp->clone_वृक्ष_Pure(असत्य);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        परत createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* rebuildSelPath(Ast_गाठ_अभिव्यक्ती* pathp, Ast_गाठ_अभिव्यक्ती* baseExprp) {
        जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(pathp, Sel)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* स्थिरांक outp
                = नवीन AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            परत outp;
        }
        परत baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
स्थिर शून्य splitDeassign(AstDeassign* nodep) {
    AstConcat* स्थिरांक concatp = VN_CAST(nodep->lhsp(), Concat);
    जर (!concatp) परत;

    File_ओळ* स्थिरांक flp = nodep->fileline();
    AstDeassign* स्थिरांक newLp = नवीन AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* स्थिरांक newRp = नवीन AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_गाठ_अभिव्यक्ती* स्थिरांक conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    VL_DO_DANGLING(conp->delete_वृक्ष(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

वर्ग ForceDiscoveryVisitor final : सार्वजनिक VNVisitorConst {
    ForceState& m_state;
    तार्किक m_inClockedActive = असत्य;

    शून्य buildForceableUnpackedArray(AstVarScope* स्थिरांक nodep,
                                     AstUnpackArrayDType* स्थिरांक arrDtypep) {
        AstVar* स्थिरांक varp = nodep->varp();
        स्थिरांक std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        स्थिरांक Ast_गाठ_DType* स्थिरांक leafDtypep = dims.back()->subDTypep()->skipRefp();
        स्थिरांक AstBasicDType* स्थिरांक innerBasicp = leafDtypep->basicp();
        जर (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            परत;
        }

        File_ओळ* स्थिरांक flp = varp->fileline();
        स्थिरांक पूर्णांक innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* स्थिरांक enVscp = info.m_forceEnVscp;
        AstVarScope* स्थिरांक valVscp = info.m_forceValVscp;

        AstSenItem* स्थिरांक itemsp = नवीन AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  नवीन AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* स्थिरांक activep = नवीन AstActive{flp, "force-update", नवीन Ast_Sen_वृक्ष{flp, itemsp}};
        activep->sen_वृक्ष_Storep(activep->sentreep());

        Ast_गाठ_Stmt* स्थिरांक alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](स्थिरांक std::vector<पूर्णांक>& idx, पूर्णांक flat) -> Ast_गाठ_Stmt* {
                AstVarRef* स्थिरांक origRefp = नवीन AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक enSelp = ForceState::buildNestedArraySel(
                    flp, नवीन AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक valSelp = ForceState::buildNestedArraySel(
                    flp, नवीन AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक forceExprp = नवीन AstOr{
                    flp, नवीन AstAnd{flp, enSelp, valSelp},
                    नवीन AstAnd{flp, नवीन AstNot{flp, enSelp->clone_वृक्ष_Pure(असत्य)}, origSelp}};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsSelp = ForceState::buildNestedArraySel(
                    flp, नवीन AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* स्थिरांक forceAssignp = नवीन AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(सत्य);

                Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsClonep = forceExprp->clone_वृक्ष_Pure(असत्य);
                rhsClonep->foreach([varp](AstVarRef* स्थिरांक r) {
                    जर (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/सत्य, /*skipRdRefresh=*/सत्य);
                परत forceAssignp;
            });
        activep->addStmtsp(नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    शून्य visit(AstAssignForce* nodep) override {
        जर (nodep->user2()) परत;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* स्थिरांक lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* स्थिरांक forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, सत्य);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsExprp = nodep->rhsp()->clone_वृक्ष_Pure(असत्य);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    शून्य visit(AstAssign* nodep) override {
        जर (m_state.doingAssign() && m_inClockedActive) {
            जर (AstVarRef* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
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
        जर (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            जर (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                परत;
            }

            जर (AstUnpackArrayDType* स्थिरांक arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                परत;
            }

            स्थिरांक AstBasicDType* स्थिरांक bdtypep = nodep->varp()->basicp();
            जर (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* स्थिरांक varp = nodep->varp();
            File_ओळ* स्थिरांक flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* स्थिरांक enVscp = info.m_forceEnVscp;
            AstVarScope* स्थिरांक valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* स्थिरांक itemsp = नवीन AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      नवीन AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* स्थिरांक activep
                = नवीन AstActive{flp, "force-update", नवीन Ast_Sen_वृक्ष{flp, itemsp}};
            activep->sen_वृक्ष_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* स्थिरांक enRefp = नवीन AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* स्थिरांक valRefp = नवीन AstVarRef{flp, valVscp, VAccess::READ};
            स्थिरांक AstBasicDType* स्थिरांक basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstOr{
                            flp, नवीन AstAnd{flp, enRefp, valRefp},
                            नवीन AstAnd{flp, नवीन AstNot{flp, enRefp->clone_वृक्ष_Pure(असत्य)}, refp}})
                      : static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* स्थिरांक forceAssignp
                = नवीन AstAssignForce{flp, नवीन AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(सत्य);
            activep->addStmtsp(नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsClonep = forceExprp->clone_वृक्ष_Pure(असत्य);
            rhsClonep->foreach([varp](AstVarRef* स्थिरांक refp) {
                जर (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            स्थिरांक तार्किक bitwiseVar = ForceState::isBitwiseDType(varp);
            स्थिरांक पूर्णांक padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            पूर्णांक rangeLsb = 0;
            पूर्णांक rangeMsb = padMsb;
            जर (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, असत्य);
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

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

        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp();
        AstVarRef* स्थिरांक lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* स्थिरांक forcedVarp = lhsVarRefp->varp();

        स्थिरांक ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        स्थिरांक ForceState::VarForceInfo* स्थिरांक varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_ओळ* स्थिरांक flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* स्थिरांक rhsAssignp
            = नवीन AstAssign{flp, नवीन AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_वृक्ष_Pure(असत्य)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        स्थिरांक तार्किक bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        जर (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_गाठ_अभिव्यक्ती* baseExprp = nodep->rhsp()->clone_वृक्ष_Pure(असत्य);
            baseExprp->foreach(
                [](AstVarRef* स्थिरांक refp) { ForceState::markNonReplaceable(refp); });
            जर (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            जर (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* स्थिरांक maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक valReadp
                    = नवीन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक valWritep
                    = नवीन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notMaskp = नवीन AstNot{flp, maskConstp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक maskedOldp = नवीन AstAnd{flp, valReadp, notMaskp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newValp = नवीन AstOr{flp, maskedOldp, baseExprp};
                valAssignp = नवीन AstAssign{flp, valWritep, newValp};

                Ast_गाठ_अभिव्यक्ती* स्थिरांक enReadp
                    = नवीन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक enWritep
                    = नवीन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newEnp
                    = नवीन AstOr{flp, enReadp, maskConstp->clone_वृक्ष_Pure(असत्य)};
                enAssignp = नवीन AstAssign{flp, enWritep, newEnp};
            } अन्यथा {
                AstConst* स्थिरांक oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsValp = ForceState::cast_To_गाठ_DType(baseExprp, forcedVarp);
                valAssignp = नवीन AstAssign{
                    flp, नवीन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = नवीन AstAssign{
                    flp, नवीन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        स्थिरांक AstSel* स्थिरांक selLhsp = VN_CAST(lhsp, Sel);
        स्थिरांक तार्किक arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsDatap = ForceState::build_Rhs_Data_अभिव्यक्ती(flp, info);
        AstCExpr* स्थिरांक rhsAddrp = नवीन AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* स्थिरांक addForceCallp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        जर (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_गाठ_Stmt* स्थिरांक stmtp = addForceCallp->makeStmt();

        Ast_गाठ* tailp = rhsAssignp;
        जर (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        जर (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        जर (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp();
        AstVarRef* स्थिरांक lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* स्थिरांक releasedVarp = lhsVarRefp->varp();

        स्थिरांक ForceState::VarForceInfo* स्थिरांक varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        जर (!varInfo) {
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_ओळ* स्थिरांक flp = nodep->fileline();

        स्थिरांक ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, असत्य);

        स्थिरांक AstSel* स्थिरांक selLhsp = VN_CAST(lhsp, Sel);
        स्थिरांक तार्किक arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* स्थिरांक releaseCallp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        जर (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_गाठ_Stmt* स्थिरांक releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        जर (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक enWritep
                = नवीन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            जर (ForceState::isBitwiseDType(releasedVarp)) {
                स्थिरांक पूर्णांक varWidth = releasedVarp->width();
                जर (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = नवीन AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } अन्यथा {
                    // forceEn = forceEn & ~mask(range);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक enReadp
                        = नवीन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* स्थिरांक maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक newEnp
                        = नवीन AstAnd{flp, enReadp, नवीन AstNot{flp, maskConst}};
                    clearEnp = नवीन AstAssign{flp, enWritep, newEnp};
                }
            } अन्यथा {
                clearEnp = नवीन AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        स्थिरांक AstSel* स्थिरांक selp = VN_CAST(lhsp, Sel);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक basep = selp ? selp->fromp() : lhsp;

        Ast_गाठ* stmtListp = releasep;
        जर (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        स्थिरांक तार्किक fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        जर (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            स्थिरांक तार्किक hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_गाठ_अभिव्यक्ती* forceReadp = nullptr;
            जर (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } अन्यथा जर (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_अभिव्यक्ती(flp, rangeInfo.m_arrayInfo));
                जर (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } अन्यथा {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* स्थिरांक assignp = नवीन AstAssign{flp, lhsp->clone_वृक्ष_Pure(असत्य), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        जर (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

वर्ग ForceReplaceVisitor final : सार्वजनिक VNVisitor {
    स्थिरांक ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_गाठ_Stmt* m_stmtp = nullptr;
    तार्किक m_inLogic = असत्य;

    शून्य iterateLogic(Ast_गाठ* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        जर (AstVarRef* स्थिरांक lhsp = VN_CAST(nodep->lhsp()->baseFromp(सत्य), VarRef)) {
            जर (Ast_गाठ* स्थिरांक updatep
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
    शून्य visit(Ast_गाठ_Procedure* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstAlways* nodep) override {
        जर (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            परत;
        }
        iterateLogic(nodep);
    }
    शून्य visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* स्थिरांक refp = VN_CAST(nodep->fromp(), VarRef);
        जर (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_गाठ*>(nodep));
            परत;
        }

        AstVar* स्थिरांक varp = refp->varp();
        स्थिरांक ForceState::VarForceInfo* स्थिरांक varInfo = m_state.getVarInfo(refp->varScopep());
        जर (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_गाठ*>(nodep));
            परत;
        }

        जर (स्थिरांक AstConst* स्थिरांक lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            स्थिरांक पूर्णांक selLsb = lsbConstp->toSInt();
            स्थिरांक पूर्णांक selMsb = selLsb + nodep->width() - 1;
            जर (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_गाठ*>(nodep));
                परत;
            }
        }

        File_ओळ* स्थिरांक flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* स्थिरांक refClonep = refp->clone_वृक्ष_Pure(असत्य);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
            flp, नवीन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_वृक्ष_Pure(असत्य));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstArraySel* nodep) override {
        जर (स्थिरांक AstArraySel* स्थिरांक backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            जर (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                परत;
            }
        }

        Ast_गाठ* स्थिरांक basep = nodep->baseFromp(सत्य);
        AstVarRef* स्थिरांक baseRefp = VN_CAST(basep, VarRef);
        जर (!baseRefp) {
            iterateChildren(nodep);
            परत;
        }
        AstVar* स्थिरांक varp = baseRefp->varp();
        स्थिरांक ForceState::VarForceInfo* स्थिरांक varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        जर (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            परत;
        }

        जर (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            परत;
        }
        जर (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            परत;
        }
        स्थिरांक ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        साठी (AstArraySel* स्थिरांक selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_गाठ_अभिव्यक्ती* स्थिरांक indexExprp
            = ForceState::build_Flatten_Index_अभिव्यक्ती(nodep->fileline(), arrayInfo);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        जर (m_state.isNotReplaceable(nodep)) परत;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        जर (स्थिरांक AstArraySel* स्थिरांक backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            जर (backSelp->fromp() == nodep) परत;
        }

        स्थिरांक ForceState::VarForceInfo* स्थिरांक varInfo = m_state.getVarInfo(nodep->varScopep());
        जर (!varInfo) परत;

        जर (varInfo->m_forceRdVscp) {
            जर (nodep->access().isRW()) {
                जर (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                परत;
            }
            जर (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_गाठ_अभिव्यक्ती* स्थिरांक readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            }
            जर (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            परत;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        जर (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            परत;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        जर (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } अन्यथा जर (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(Ast_गाठ* nodep) override {
        जर (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_AS(nodep, गाठ_अभिव्यक्ती);
            Ast_गाठ* स्थिरांक basep = exprp->baseFromp(सत्य);
            AstVarRef* स्थिरांक baseRefp = VN_CAST(basep, VarRef);
            जर (baseRefp) {
                AstVar* स्थिरांक varp = baseRefp->varp();
                जर (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    स्थिरांक ForceState::VarForceInfo* स्थिरांक varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    जर (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        स्थिरांक पूर्णांक forcePathIndex = varInfo->findForcePathIndex(exprp);
                        जर (forcePathIndex >= 0) {
                            जर (!baseRefp->access().isReadOnly()) परत;
                            Ast_गाठ_अभिव्यक्ती* स्थिरांक readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                            परत;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    explicit ForceReplaceVisitor(AstNetlist* nodep, स्थिरांक ForceState& state)
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
    जर (!v3Global.hasForceableSignals()) परत;
    ForceState state{असत्य, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्ष("force", 0, dump_वृक्ष_Either_Level() >= 3);
}

स्थिर शून्य assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    जर (!v3Global.hasAssignDeassign()) परत;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.ढकला_back(deassignp); });
    साठी (AstDeassign* स्थिरांक deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_गाठ_Stmt* nodep) {
        जर (AstAssignCont* स्थिरांक assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.ढकला_back(assignContp);
        } अन्यथा जर (AstDeassign* स्थिरांक deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.ढकला_back(deassignp);
        }
    });

    साठी (AstAssignCont* स्थिरांक assignp : assignContps) {
        assignp->replaceWith(नवीन AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_वृक्ष(), assignp);
    }
    साठी (AstDeassign* स्थिरांक deassignp : deassignps) {
        deassignp->replaceWith(
            नवीन AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_वृक्ष_Pure(सत्य)});
        VL_DO_DANGLING(deassignp->delete_वृक्ष(), deassignp);
    }
    ForceState state{सत्य, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्ष("assign-deassign", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3Force::forceAndAssignAll(AstNetlist* nodep) {
    स्थिरांक VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
