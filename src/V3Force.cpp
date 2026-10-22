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
        तार्किक m_hasArraySel = असत्य;  // If this has an array select on LHS
        तार्किक m_isExternal = असत्य;  // Synthetic force for the public force controls
        तार्किक m_skipRdRefresh = असत्य;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_गाँठो_अभिव्यक्ति* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = स्वभाव;
        ForceInfo(पूर्णाङ्क rangeLsb, पूर्णाङ्क rangeMsb, पूर्णाङ्क padLsb, पूर्णाङ्क padMsb, पूर्णाङ्क forceId,
                  तार्किक hasArraySel, AstVarScope* rhsVarVscp, Ast_गाँठो_अभिव्यक्ति* rhsExprp)
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
        स्थिर शून्य build_Force_Path_साँचो_Recurse(Ast_गाँठो_अभिव्यक्ति* nodep, string& out) {
            यदि (VN_IS(nodep, VarRef)) फर्काऊ;
            यदि (अचर AstSel* अचर selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_साँचो_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                फर्काऊ;
            }
            यदि (AstStructSel* अचर selp = VN_CAST(nodep, StructSel)) {
                Ast_गाँठो_अभिव्यक्ति* अचर fromp = selp->fromp();
                build_Force_Path_साँचो_Recurse(fromp, out);
                out += "|T" + selp->name();
                फर्काऊ;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        स्थिर string force_Path_साँचो(Ast_गाँठो_अभिव्यक्ति* nodep) {
            string out;
            build_Force_Path_साँचो_Recurse(nodep, out);
            फर्काऊ out;
        }

    सार्वजनिक:
        पूर्णाङ्क getOrCreateForcePathIndex(Ast_गाँठो_अभिव्यक्ति* nodep) {
            अचर स्वतः pair
                = m_forcePathToIndex.emplace(force_Path_साँचो(nodep), m_nextForcePathIndex);
            यदि (pair.second) ++m_nextForcePathIndex;
            फर्काऊ pair.first->second;
        }

        पूर्णाङ्क findForcePathIndex(Ast_गाँठो_अभिव्यक्ति* nodep) अचर {
            अचर स्वतः it = m_forcePathToIndex.find(force_Path_साँचो(nodep));
            फर्काऊ it != m_forcePathToIndex.end() ? it->second : -1;
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

गुप्त:
    using ScopeVarCache = std::unordered_map<अचर AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    अचर VNUser1InUse m_user1InUse;
    अचर VNUser2InUse m_user2InUse;

सार्वजनिक:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<अचर AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<अचर AstVarScope*>;

गुप्त:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, पूर्णाङ्क> m_varToId;
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
    स्थिर AstConst* makeZeroConst(Ast_गाँठो* nodep, पूर्णाङ्क width) {
        V3_सङ्ख्या zero{nodep, width};
        zero.setAllBits0();
        फर्काऊ नूतन AstConst{nodep->fileline(), zero};
    }

    स्थिर AstConst* makeConst32(File_पङ्क्ति* flp, पूर्णाङ्क मान) {
        फर्काऊ नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, static_cast<uint32_t>(मान)};
    }

    स्थिर AstConst* makeRangeMaskConst(Ast_गाँठो* nodep, पूर्णाङ्क width, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        V3_सङ्ख्या mask{nodep, width};
        mask.setAllBits0();
        लागि (पूर्णाङ्क bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        फर्काऊ नूतन AstConst{nodep->fileline(), mask};
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* zeroPadToBaseWidth(Ast_गाँठो_अभिव्यक्ति* exprp, पूर्णाङ्क baseWidth, पूर्णाङ्क padLsb,
                                           पूर्णाङ्क padMsb) {
        यदि (baseWidth <= 0) फर्काऊ exprp;
        अचर पूर्णाङ्क तल्लो_Pad = padLsb;
        अचर पूर्णाङ्क माथिल्लो_Pad = baseWidth - (padMsb + 1);
        यदि (तल्लो_Pad > 0) {
            exprp = नूतन AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, तल्लो_Pad)};
        }
        यदि (माथिल्लो_Pad > 0) {
            exprp = नूतन AstConcat{exprp->fileline(), makeZeroConst(exprp, माथिल्लो_Pad), exprp};
        }
        फर्काऊ exprp;
    }

    स्थिर तार्किक isUnpackedArrayDType(अचर Ast_गाँठो_DType* dtypep) {
        फर्काऊ VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    स्थिर तार्किक isBitwiseDType(अचर Ast_गाँठो* nodep) {
        अचर AstBasicDType* अचर basicp = nodep->dtypep()->skipRefp()->basicp();
        फर्काऊ basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* cast_To_गाँठो_DType(Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो* dtypeFromp) {
        अचर Ast_गाँठो_DType* अचर dtypep = dtypeFromp->dtypep()->skipRefp();
        अचर AstBasicDType* अचर basicp = dtypep->basicp();
        यदि (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            फर्काऊ exprp;
        }
        फर्काऊ नूतन AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    स्थिर शून्य markNonReplaceable(AstVarRef* अचर nodep) { nodep->user1SetOnce(); }
    तार्किक isNotReplaceable(अचर AstVarRef* अचर nodep) अचर {
        फर्काऊ nodep->user1() || m_permanentlyProtected.गन्ती(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    शून्य markPermanentlyProtected(AstVarRef* अचर nodep) अचर {
        markNonReplaceable(nodep);
        m_permanentlyProtected.घुसाउ(nodep);
    }

    स्थिर std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        लागि (स्वतः& it : info.m_forces) forceps.थप_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](अचर ForceInfo* ap, अचर ForceInfo* bp) {
            फर्काऊ ap->m_forceId < bp->m_forceId;
        });
        फर्काऊ forceps;
    }

    स्थिर std::vector<अचर ForceInfo*> forceInfosInIdOrder(अचर VarForceInfo& info) {
        std::vector<अचर ForceInfo*> forceps;
        forceps.reserve(info.m_forces.size());
        लागि (अचर स्वतः& it : info.m_forces) forceps.थप_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](अचर ForceInfo* ap, अचर ForceInfo* bp) {
            फर्काऊ ap->m_forceId < bp->m_forceId;
        });
        फर्काऊ forceps;
    }

    स्थिर तार्किक isOpaquePathSelector(अचर Ast_गाँठो* nodep) {
        फर्काऊ VN_IS(nodep, Sel) || VN_IS(nodep, गाँठो_Sel) || VN_IS(nodep, StructSel);
    }

    स्थिर तार्किक isOutermostOpaquePathSelector(अचर Ast_गाँठो* nodep) {
        यदि (!isOpaquePathSelector(nodep)) फर्काऊ असत्य;
        अचर Ast_गाँठो* अचर backp = nodep->backp();
        फर्काऊ !backp || !isOpaquePathSelector(backp);
    }

    स्थिर AstVarRef* getOneVarRef(Ast_गाँठो_अभिव्यक्ति* forceStmtp) {
        Ast_गाँठो* अचर basep = forceStmtp->baseFromp(सत्य);
        यदि (AstSampled* sampledp = VN_CAST(basep, Sampled))
            यदि (Ast_गाँठो_अभिव्यक्ति* exprp = VN_CAST(sampledp->exprp(), गाँठो_अभिव्यक्ति))
                फर्काऊ getOneVarRef(exprp);
        AstVarRef* अचर varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        फर्काऊ varRefp;
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* buildNestedArraySel(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* fromp,
                                            अचर std::vector<पूर्णाङ्क>& indicies) {
        Ast_गाँठो_अभिव्यक्ति* curp = fromp;
        लागि (अचर पूर्णाङ्क idx : indicies) curp = नूतन AstArraySel{flp, curp, idx};
        फर्काऊ curp;
    }

    ढाँचा <typename Fn>
    स्थिर Ast_गाँठो_Stmt* foreachUnpackedLeaf(अचर std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_गाँठो_Stmt* headp = nullptr;
        Ast_गाँठो_Stmt* tailp = nullptr;
        यदि (dims.empty()) फर्काऊ nullptr;
        पूर्णाङ्क total = 1;
        लागि (अचर AstUnpackArrayDType* अचर d : dims) total *= d->elementsConst();
        यदि (total <= 0) फर्काऊ nullptr;
        std::vector<पूर्णाङ्क> idx(dims.size(), 0);
        लागि (पूर्णाङ्क flat = 0; flat < total; ++flat) {
            Ast_गाँठो_Stmt* अचर stmtp = buildLeaf(idx, flat);
            यदि (!headp) {
                headp = stmtp;
            } अन्यथा {
                tailp->addNext(stmtp);
            }
            tailp = stmtp;
            लागि (पूर्णाङ्क d = static_cast<पूर्णाङ्क>(dims.size()) - 1; d >= 0; --d) {
                यदि (++idx[d] < dims[d]->elementsConst()) रोक;
                idx[d] = 0;
            }
        }
        फर्काऊ headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_गाँठो_अभिव्यक्ति* lhsp, AstVar* varp,
                                     तार्किक requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        यदि (अचर AstSel* अचर outerSelp = VN_CAST(lhsp, Sel)) {
            पूर्णाङ्क totalLsb = 0;
            लागि (Ast_गाँठो_अभिव्यक्ति* curp = lhsp; अचर AstSel* अचर selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                यदि (requireConstRangeSelect) {
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
        यदि (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<पूर्णाङ्क> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            लागि (AstArraySel* अचर selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.थप_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } अन्यथा यदि (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        यदि (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            अचर पूर्णाङ्क index = varInfo.getOrCreateForcePathIndex(lhsp);
            info.m_rangeLsb = index;
            info.m_rangeMsb = index;
        }
        फर्काऊ info;
    }

    Ast_गाँठो_अभिव्यक्ति* add_Rhs_मान_Reads(अचर VarForceInfo& varInfo, Ast_गाँठो_अभिव्यक्ति* exprp) अचर {
        यदि (!doingAssign()) फर्काऊ exprp;

        अचर std::vector<अचर ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        यदि (forceps.empty()) फर्काऊ exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* अचर cexprp = नूतन AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        लागि (अचर ForceInfo* अचर finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* अचर refp
                = नूतन AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        फर्काऊ cexprp;
    }

    Ast_गाँठो_अभिव्यक्ति* createForceReadCall(अचर VarForceInfo& varInfo, File_पङ्क्ति* flp, VCMethod method,
                                     Ast_गाँठो_अभिव्यक्ति* originalExprp, Ast_गाँठो* dtypeFromp,
                                     Ast_गाँठो_अभिव्यक्ति* indexExprp) अचर {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_गाँठो_अभिव्यक्ति* अचर origValp
            = add_Rhs_मान_Reads(varInfo, cast_To_गाँठो_DType(originalExprp, dtypeFromp));

        AstCMethodHard* अचर callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        यदि (indexExprp) callp->addPinsp(indexExprp);
        callp->dtypeFrom(dtypeFromp);
        फर्काऊ callp;
    }

    Ast_गाँठो_अभिव्यक्ति* createForceRdExpression(अचर VarForceInfo& varInfo, File_पङ्क्ति* flp) अचर {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* अचर varp = varInfo.m_varVscp->varp();
        यदि (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            यदि (doingAssign()) { फर्काऊ नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सत्य);
            AstVarRef* अचर baseRefp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            फर्काऊ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* अचर baseRefp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_गाँठो_अभिव्यक्ति* अचर enRefp = नूतन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_गाँठो_अभिव्यक्ति* अचर valRefp = नूतन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        यदि (isBitwiseDType(varp)) {
            फर्काऊ नूतन AstOr{
                flp, नूतन AstAnd{flp, enRefp, valRefp},
                नूतन AstAnd{flp, नूतन AstNot{flp, enRefp->clone_रूख_Pure(असत्य)}, baseRefp}};
        }
        फर्काऊ नूतन AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_गाँठो_Stmt* createForceRdUpdateStmt(अचर VarForceInfo& varInfo) अचर {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* अचर varp = varInfo.m_varVscp->varp();
        यदि (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            फर्काऊ createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_पङ्क्ति* अचर flp = varInfo.m_varVscp->fileline();
        फर्काऊ नूतन AstAssign{flp, नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_गाँठो_Stmt* createForceRdUpdateStmtUnpacked(अचर VarForceInfo& varInfo) अचर {
        File_पङ्क्ति* अचर flp = varInfo.m_varVscp->fileline();
        AstVar* अचर varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* अचर arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        अचर std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_गाँठो_Stmt* अचर stmtsp = foreachUnpackedLeaf(
            dims, [&](अचर std::vector<पूर्णाङ्क>& idx, पूर्णाङ्क /*flat*/) -> Ast_गाँठो_Stmt* {
                AstVarRef* अचर baseRefp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_गाँठो_अभिव्यक्ति* अचर baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_गाँठो_अभिव्यक्ति* अचर enSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_गाँठो_अभिव्यक्ति* अचर valSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_गाँठो_अभिव्यक्ति* अचर readExprp = नूतन AstOr{
                    flp, नूतन AstAnd{flp, enSelp, valSelp},
                    नूतन AstAnd{flp, नूतन AstNot{flp, enSelp->clone_रूख_Pure(असत्य)}, baseSelp}};
                Ast_गाँठो_अभिव्यक्ति* अचर rdLhsSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                फर्काऊ नूतन AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        लागि (अचर ForceInfo* अचर finfop : forceInfosInIdOrder(varInfo)) {
            यदि (!finfop->m_isExternal) जारी;
            stmtsp->addNext(नूतन AstAssign{flp,
                                          नूतन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_रूख_Pure(असत्य)});
        }
        फर्काऊ stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        अचर स्वतः it = m_varToId.find(vscp);
        यदि (it != m_varToId.end()) फर्काऊ m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.size());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* अचर varp = info.m_varp;
        यदि (!varp->isForceable()) फर्काऊ info;

        File_पङ्क्ति* अचर flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        अचर तार्किक helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        यदि (!helperVarsBuilt) {
            अचर तार्किक unpacked = isUnpackedArrayDType(varp->dtypep());
            अचर VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_गाँठो_DType* अचर enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = नूतन AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(सत्य);
            helperVars.m_enVarp
                = नूतन AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(सत्य);
            helperVars.m_valVarp
                = नूतन AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(सत्य);
            varp->addNextHere(helperVars.m_rdVarp);
            varp->addNextHere(helperVars.m_enVarp);
            varp->addNextHere(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        यदि (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } अन्यथा {
            info.m_forceRdVscp = नूतन AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = नूतन AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = नूतन AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        फर्काऊ info;
    }

    शून्य markClockedWrite(AstVar* varp) { m_clockedWrites.घुसाउ(varp); }
    तार्किक hasClockedWrite(AstVar* varp) अचर { फर्काऊ m_clockedWrites.गन्ती(varp); }

    तार्किक doingAssign() अचर { फर्काऊ m_doingAssign; }

    अचर VarForceInfo* getVarInfo(AstVarScope* vscp) अचर {
        अचर स्वतः it = m_varToId.find(vscp);
        फर्काऊ it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, अचर AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        यदि (cache.empty()) {
            लागि (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        अचर स्वतः it = cache.find(varp);
        फर्काऊ it != cache.end() ? it->second : nullptr;
    }
    शून्य addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_गाँठो_अभिव्यक्ति* rhsExprp,
                            AstAssignForce* forceStmtp, पूर्णाङ्क rangeLsb, पूर्णाङ्क rangeMsb, पूर्णाङ्क padLsb,
                            पूर्णाङ्क padMsb, तार्किक hasArraySel, तार्किक skipRdRefresh = असत्य) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        अचर पूर्णाङ्क forceId = info.m_forces.size();
        File_पङ्क्ति* अचर flp = varp->fileline();
        AstScope* अचर scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        यदि (!info.m_forceVecVscp) {
            AstCDType* अचर forceVecDtypep = नूतन AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* अचर forceVecVarp
                = नूतन AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(असत्य);
            forceVecVarp->isInternal(सत्य);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = नूतन AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        स्वतः pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        यदि (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* अचर refp) {
                यदि (!refp->access().isReadOnly()) फर्काऊ;
                AstVar* अचर depVarp = refp->varp();
                यदि (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.थप_back(depVarp);
                }
            });
            लागि (AstVar* अचर depVarp : depVarps) m_rhsDepToForces[depVarp].थप_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    स्थिर शून्य collectArraySelInfo(Ast_गाँठो_अभिव्यक्ति* exprp, ArraySelInfo& info) {
        यदि (अचर स्वतः* अचर selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = सत्य;
            collectArraySelInfo(selp->fromp(), info);
        } अन्यथा यदि (स्वतः* अचर selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.थप_back(selp);
        } अन्यथा यदि (अचर स्वतः* अचर memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    स्थिर ArraySelInfo getArraySelInfo(Ast_गाँठो_अभिव्यक्ति* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        फर्काऊ info;
    }

    स्थिर std::vector<पूर्णाङ्क> arraySelDimSizes(अचर ArraySelInfo& info) {
        std::vector<पूर्णाङ्क> dims;
        dims.reserve(info.m_sels.size());
        लागि (AstArraySel* अचर selp : info.m_sels) {
            Ast_गाँठो_DType* अचर dtypep = selp->fromp()->dtypep()->skipRefp();
            अचर AstUnpackArrayDType* अचर arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.थप_back(arrayp->declRange().elements());
        }
        फर्काऊ dims;
    }

    स्थिर पूर्णाङ्क flattenIndex(अचर std::vector<पूर्णाङ्क>& indices, अचर std::vector<पूर्णाङ्क>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        पूर्णाङ्क index = 0;
        पूर्णाङ्क stride = 1;
        लागि (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        फर्काऊ index;
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* build_Flatten_Index_अभिव्यक्ति(File_पङ्क्ति* flp, अचर ArraySelInfo& info) {
        अचर std::vector<पूर्णाङ्क> dimSizes = arraySelDimSizes(info);
        तार्किक allConst = सत्य;
        लागि (AstArraySel* अचर selp : info.m_sels) {
            यदि (!VN_IS(selp->bitp(), Const)) {
                allConst = असत्य;
                रोक;
            }
        }
        यदि (allConst) {
            std::vector<पूर्णाङ्क> constIndices;
            constIndices.reserve(info.m_sels.size());
            लागि (AstArraySel* अचर selp : info.m_sels) {
                constIndices.थप_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            फर्काऊ makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_गाँठो_अभिव्यक्ति* resultp = nullptr;
        पूर्णाङ्क stride = 1;
        लागि (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(info.m_sels.size()) - 1; i >= 0; --i) {
            Ast_गाँठो_अभिव्यक्ति* termp = info.m_sels[i]->bitp()->clone_रूख_Pure(असत्य);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            यदि (termp->width() < 32) termp = नूतन AstExtend{flp, termp, 32};
            यदि (stride != 1) termp = नूतन AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? नूतन AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        फर्काऊ resultp;
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* build_Rhs_Data_अभिव्यक्ति(File_पङ्क्ति* flp, अचर ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        फर्काऊ नूतन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    शून्य finalizeRhsVars() {
        लागि (VarForceInfo& info : m_varInfos) {
            AstVar* अचर varp = info.m_varp;
            यदि (info.m_forces.empty()) जारी;

            AstScope* अचर scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_पङ्क्ति* अचर flp = varp->fileline();
            अचर std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            लागि (ForceInfo* अचर finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* अचर rhsVarp = नूतन AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(सत्य);
                rhsVarp->sigPublic(सत्य);
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
                AstAssign* अचर rhsAssignp = नूतन AstAssign{
                    flp, नूतन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                यदि (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* अचर touchCallp = नूतन AstCMethodHard{
                        flp, नूतन AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_गाँठो_Stmt* अचर touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->addNextHere(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* अचर alwaysp
                    = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_रूख* अचर senTreep
                    = नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* अचर activep = नूतन AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_रूख_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            यदि (info.m_forceRdVscp) {
                यदि (!m_forceRdUpdateBuilt.घुसाउ(info.m_varVscp).second) जारी;
                AstActive* अचर activeInitp = नूतन AstActive{
                    flp, "force-init",
                    नूतन Ast_Sen_रूख{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_रूख_Storep(activeInitp->sentreep());
                Ast_गाँठो_Stmt* initStmtp = nullptr;
                यदि (AstUnpackArrayDType* अचर arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    अचर std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    अचर पूर्णाङ्क innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](अचर std::vector<पूर्णाङ्क>& idx, पूर्णाङ्क /*flat*/) -> Ast_गाँठो_Stmt* {
                            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = buildNestedArraySel(
                                flp, नूतन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            फर्काऊ नूतन AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } अन्यथा {
                    initStmtp = नूतन AstAssign{
                        flp, नूतन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->addNext(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(नूतन AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                स्वतः addSenItem = [&](AstVarScope* vscp) {
                    यदि (!vscp) फर्काऊ;
                    AstSenItem* अचर nextp = नूतन AstSenItem{
                        flp, VEdgeType::ET_CHANGED, नूतन AstVarRef{flp, vscp, VAccess::READ}};
                    यदि (itemsp) {
                        itemsp->addNext(nextp);
                    } अन्यथा {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* अचर origSenRefp = नूतन AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* अचर origItemp
                    = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                यदि (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->addNext(origItemp);
                लागि (ForceInfo* अचर finfop : forceps) {
                    यदि (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* अचर activep
                    = नूतन AstActive{flp, "force-rd-update", नूतन Ast_Sen_रूख{flp, itemsp}};
                activep->sen_रूख_Storep(activep->sentreep());
                activep->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_गाँठो* createRhsUpdatesForWrite(File_पङ्क्ति* flp, AstVar* writtenVarp) अचर {
        यदि (!doingAssign()) फर्काऊ nullptr;

        अचर स्वतः it = m_rhsDepToForces.find(writtenVarp);
        यदि (it == m_rhsDepToForces.end()) फर्काऊ nullptr;

        Ast_गाँठो* headp = nullptr;
        Ast_गाँठो* tailp = nullptr;
        लागि (अचर ForceInfo* अचर finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* अचर updatep
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_रूख_Pure(असत्य)};
            यदि (tailp) {
                tailp->addNextHere(updatep);
            } अन्यथा {
                headp = updatep;
            }
            tailp = updatep;
        }
        फर्काऊ headp;
    }

    अचर ForceInfo& getForceInfo(AstAssignForce* forceStmtp) अचर {
        AstVarScope* अचर vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        अचर VarForceInfo* अचर varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        स्वतः it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        फर्काऊ it2->second;
    }

    स्थिर तार्किक selOverlapsAnyForce(अचर VarForceInfo& varInfo, पूर्णाङ्क selLsb, पूर्णाङ्क selMsb) {
        लागि (अचर स्वतः& pair : varInfo.m_forces) {
            यदि (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    Ast_गाँठो_अभिव्यक्ति* createForceReadExpression(अचर VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) अचर {
        File_पङ्क्ति* अचर flp = originalRefp->fileline();
        AstVarRef* अचर refp = originalRefp->clone_रूख_Pure(असत्य);
        refp->access(VAccess::READ);
        फर्काऊ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_गाँठो_अभिव्यक्ति* createForceReadIndexExpression(अचर VarForceInfo& varInfo,
                                                Ast_गाँठो_अभिव्यक्ति* originalExprp,
                                                Ast_गाँठो_अभिव्यक्ति* indexExprp) अचर {
        File_पङ्क्ति* अचर flp = originalExprp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = originalExprp->clone_रूख_Pure(असत्य);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        फर्काऊ createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_INDEX, exprp, originalExprp,
                                   indexExprp);
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* rebuildSelPath(Ast_गाँठो_अभिव्यक्ति* pathp, Ast_गाँठो_अभिव्यक्ति* baseExprp) {
        यदि (अचर AstSel* अचर selp = VN_CAST(pathp, Sel)) {
            Ast_गाँठो_अभिव्यक्ति* अचर fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* अचर outp
                = नूतन AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            फर्काऊ outp;
        }
        फर्काऊ baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
स्थिर शून्य splitDeassign(AstDeassign* nodep) {
    AstConcat* अचर concatp = VN_CAST(nodep->lhsp(), Concat);
    यदि (!concatp) फर्काऊ;

    File_पङ्क्ति* अचर flp = nodep->fileline();
    AstDeassign* अचर newLp = नूतन AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* अचर newRp = नूतन AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_गाँठो_अभिव्यक्ति* अचर conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->addNextHere(newRp);
    VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    VL_DO_DANGLING(conp->delete_रूख(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

वर्ग ForceDiscoveryVisitor final : सार्वजनिक VNVisitorConst {
    ForceState& m_state;
    तार्किक m_inClockedActive = असत्य;

    शून्य buildForceableUnpackedArray(AstVarScope* अचर nodep,
                                     AstUnpackArrayDType* अचर arrDtypep) {
        AstVar* अचर varp = nodep->varp();
        अचर std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        अचर Ast_गाँठो_DType* अचर leafDtypep = dims.back()->subDTypep()->skipRefp();
        अचर AstBasicDType* अचर innerBasicp = leafDtypep->basicp();
        यदि (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            फर्काऊ;
        }

        File_पङ्क्ति* अचर flp = varp->fileline();
        अचर पूर्णाङ्क innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* अचर enVscp = info.m_forceEnVscp;
        AstVarScope* अचर valVscp = info.m_forceValVscp;

        AstSenItem* अचर itemsp = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  नूतन AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* अचर activep = नूतन AstActive{flp, "force-update", नूतन Ast_Sen_रूख{flp, itemsp}};
        activep->sen_रूख_Storep(activep->sentreep());

        Ast_गाँठो_Stmt* अचर alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](अचर std::vector<पूर्णाङ्क>& idx, पूर्णाङ्क flat) -> Ast_गाँठो_Stmt* {
                AstVarRef* अचर origRefp = नूतन AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_गाँठो_अभिव्यक्ति* अचर origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_गाँठो_अभिव्यक्ति* अचर enSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_गाँठो_अभिव्यक्ति* अचर valSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_गाँठो_अभिव्यक्ति* अचर forceExprp = नूतन AstOr{
                    flp, नूतन AstAnd{flp, enSelp, valSelp},
                    नूतन AstAnd{flp, नूतन AstNot{flp, enSelp->clone_रूख_Pure(असत्य)}, origSelp}};
                Ast_गाँठो_अभिव्यक्ति* अचर lhsSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* अचर forceAssignp = नूतन AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(सत्य);

                Ast_गाँठो_अभिव्यक्ति* अचर rhsClonep = forceExprp->clone_रूख_Pure(असत्य);
                rhsClonep->foreach([varp](AstVarRef* अचर r) {
                    यदि (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/सत्य, /*skipRdRefresh=*/सत्य);
                फर्काऊ forceAssignp;
            });
        activep->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    शून्य visit(AstAssignForce* nodep) override {
        यदि (nodep->user2()) फर्काऊ;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* अचर lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* अचर forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, सत्य);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_गाँठो_अभिव्यक्ति* अचर rhsExprp = nodep->rhsp()->clone_रूख_Pure(असत्य);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    शून्य visit(AstAssign* nodep) override {
        यदि (m_state.doingAssign() && m_inClockedActive) {
            यदि (AstVarRef* अचर lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
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
        यदि (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            यदि (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                फर्काऊ;
            }

            यदि (AstUnpackArrayDType* अचर arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                फर्काऊ;
            }

            अचर AstBasicDType* अचर bdtypep = nodep->varp()->basicp();
            यदि (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* अचर varp = nodep->varp();
            File_पङ्क्ति* अचर flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* अचर enVscp = info.m_forceEnVscp;
            AstVarScope* अचर valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* अचर itemsp = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      नूतन AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* अचर activep
                = नूतन AstActive{flp, "force-update", नूतन Ast_Sen_रूख{flp, itemsp}};
            activep->sen_रूख_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* अचर refp = नूतन AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* अचर enRefp = नूतन AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* अचर valRefp = नूतन AstVarRef{flp, valVscp, VAccess::READ};
            अचर AstBasicDType* अचर basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_गाँठो_अभिव्यक्ति* अचर forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstOr{
                            flp, नूतन AstAnd{flp, enRefp, valRefp},
                            नूतन AstAnd{flp, नूतन AstNot{flp, enRefp->clone_रूख_Pure(असत्य)}, refp}})
                      : static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* अचर forceAssignp
                = नूतन AstAssignForce{flp, नूतन AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(सत्य);
            activep->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_गाँठो_अभिव्यक्ति* अचर rhsClonep = forceExprp->clone_रूख_Pure(असत्य);
            rhsClonep->foreach([varp](AstVarRef* अचर refp) {
                यदि (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            अचर तार्किक bitwiseVar = ForceState::isBitwiseDType(varp);
            अचर पूर्णाङ्क padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            पूर्णाङ्क rangeLsb = 0;
            पूर्णाङ्क rangeMsb = padMsb;
            यदि (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, असत्य);
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

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

        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp();
        AstVarRef* अचर lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* अचर forcedVarp = lhsVarRefp->varp();

        अचर ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        अचर ForceState::VarForceInfo* अचर varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_पङ्क्ति* अचर flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* अचर rhsAssignp
            = नूतन AstAssign{flp, नूतन AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_रूख_Pure(असत्य)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        अचर तार्किक bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        यदि (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_गाँठो_अभिव्यक्ति* baseExprp = nodep->rhsp()->clone_रूख_Pure(असत्य);
            baseExprp->foreach(
                [](AstVarRef* अचर refp) { ForceState::markNonReplaceable(refp); });
            यदि (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            यदि (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* अचर maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_गाँठो_अभिव्यक्ति* अचर valReadp
                    = नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_गाँठो_अभिव्यक्ति* अचर valWritep
                    = नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_गाँठो_अभिव्यक्ति* अचर notMaskp = नूतन AstNot{flp, maskConstp};
                Ast_गाँठो_अभिव्यक्ति* अचर maskedOldp = नूतन AstAnd{flp, valReadp, notMaskp};
                Ast_गाँठो_अभिव्यक्ति* अचर newValp = नूतन AstOr{flp, maskedOldp, baseExprp};
                valAssignp = नूतन AstAssign{flp, valWritep, newValp};

                Ast_गाँठो_अभिव्यक्ति* अचर enReadp
                    = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_गाँठो_अभिव्यक्ति* अचर enWritep
                    = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_गाँठो_अभिव्यक्ति* अचर newEnp
                    = नूतन AstOr{flp, enReadp, maskConstp->clone_रूख_Pure(असत्य)};
                enAssignp = नूतन AstAssign{flp, enWritep, newEnp};
            } अन्यथा {
                AstConst* अचर oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_गाँठो_अभिव्यक्ति* अचर rhsValp = ForceState::cast_To_गाँठो_DType(baseExprp, forcedVarp);
                valAssignp = नूतन AstAssign{
                    flp, नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = नूतन AstAssign{
                    flp, नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        अचर AstSel* अचर selLhsp = VN_CAST(lhsp, Sel);
        अचर तार्किक arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsDatap = ForceState::build_Rhs_Data_अभिव्यक्ति(flp, info);
        AstCExpr* अचर rhsAddrp = नूतन AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* अचर addForceCallp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        यदि (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_गाँठो_Stmt* अचर stmtp = addForceCallp->makeStmt();

        Ast_गाँठो* tailp = rhsAssignp;
        यदि (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        यदि (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        यदि (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp();
        AstVarRef* अचर lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* अचर releasedVarp = lhsVarRefp->varp();

        अचर ForceState::VarForceInfo* अचर varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        यदि (!varInfo) {
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
            फर्काऊ;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_पङ्क्ति* अचर flp = nodep->fileline();

        अचर ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, असत्य);

        अचर AstSel* अचर selLhsp = VN_CAST(lhsp, Sel);
        अचर तार्किक arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* अचर releaseCallp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        यदि (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_गाँठो_Stmt* अचर releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        यदि (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_गाँठो_अभिव्यक्ति* अचर enWritep
                = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            यदि (ForceState::isBitwiseDType(releasedVarp)) {
                अचर पूर्णाङ्क varWidth = releasedVarp->width();
                यदि (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = नूतन AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } अन्यथा {
                    // forceEn = forceEn & ~mask(range);
                    Ast_गाँठो_अभिव्यक्ति* अचर enReadp
                        = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* अचर maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_गाँठो_अभिव्यक्ति* अचर newEnp
                        = नूतन AstAnd{flp, enReadp, नूतन AstNot{flp, maskConst}};
                    clearEnp = नूतन AstAssign{flp, enWritep, newEnp};
                }
            } अन्यथा {
                clearEnp = नूतन AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        अचर AstSel* अचर selp = VN_CAST(lhsp, Sel);
        Ast_गाँठो_अभिव्यक्ति* अचर basep = selp ? selp->fromp() : lhsp;

        Ast_गाँठो* stmtListp = releasep;
        यदि (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        अचर तार्किक fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        यदि (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            अचर तार्किक hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_गाँठो_अभिव्यक्ति* forceReadp = nullptr;
            यदि (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } अन्यथा यदि (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::build_Flatten_Index_अभिव्यक्ति(flp, rangeInfo.m_arrayInfo));
                यदि (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } अन्यथा {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* अचर assignp = नूतन AstAssign{flp, lhsp->clone_रूख_Pure(असत्य), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        यदि (varInfo->m_forceRdVscp) stmtListp->addNext(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

वर्ग ForceReplaceVisitor final : सार्वजनिक VNVisitor {
    अचर ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_गाँठो_Stmt* m_stmtp = nullptr;
    तार्किक m_inLogic = असत्य;

    शून्य iterateLogic(Ast_गाँठो* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठो_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        यदि (AstVarRef* अचर lhsp = VN_CAST(nodep->lhsp()->baseFromp(सत्य), VarRef)) {
            यदि (Ast_गाँठो* अचर updatep
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
    शून्य visit(Ast_गाँठो_Procedure* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstAlways* nodep) override {
        यदि (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            फर्काऊ;
        }
        iterateLogic(nodep);
    }
    शून्य visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    शून्य visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* अचर refp = VN_CAST(nodep->fromp(), VarRef);
        यदि (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_गाँठो*>(nodep));
            फर्काऊ;
        }

        AstVar* अचर varp = refp->varp();
        अचर ForceState::VarForceInfo* अचर varInfo = m_state.getVarInfo(refp->varScopep());
        यदि (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_गाँठो*>(nodep));
            फर्काऊ;
        }

        यदि (अचर AstConst* अचर lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            अचर पूर्णाङ्क selLsb = lsbConstp->toSInt();
            अचर पूर्णाङ्क selMsb = selLsb + nodep->width() - 1;
            यदि (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_गाँठो*>(nodep));
                फर्काऊ;
            }
        }

        File_पङ्क्ति* अचर flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* अचर refClonep = refp->clone_रूख_Pure(असत्य);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* अचर callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_रूख_Pure(असत्य));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstArraySel* nodep) override {
        यदि (अचर AstArraySel* अचर backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            यदि (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                फर्काऊ;
            }
        }

        Ast_गाँठो* अचर basep = nodep->baseFromp(सत्य);
        AstVarRef* अचर baseRefp = VN_CAST(basep, VarRef);
        यदि (!baseRefp) {
            iterateChildren(nodep);
            फर्काऊ;
        }
        AstVar* अचर varp = baseRefp->varp();
        अचर ForceState::VarForceInfo* अचर varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        यदि (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            फर्काऊ;
        }

        यदि (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            फर्काऊ;
        }
        यदि (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            फर्काऊ;
        }
        अचर ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        लागि (AstArraySel* अचर selp : arrayInfo.m_sels) iterateAndNextNull(selp->bitp());
        Ast_गाँठो_अभिव्यक्ति* अचर indexExprp
            = ForceState::build_Flatten_Index_अभिव्यक्ति(nodep->fileline(), arrayInfo);
        Ast_गाँठो_अभिव्यक्ति* अचर readExprp
            = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (m_state.isNotReplaceable(nodep)) फर्काऊ;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        यदि (अचर AstArraySel* अचर backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            यदि (backSelp->fromp() == nodep) फर्काऊ;
        }

        अचर ForceState::VarForceInfo* अचर varInfo = m_state.getVarInfo(nodep->varScopep());
        यदि (!varInfo) फर्काऊ;

        यदि (varInfo->m_forceRdVscp) {
            यदि (nodep->access().isRW()) {
                यदि (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                फर्काऊ;
            }
            यदि (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_गाँठो_अभिव्यक्ति* अचर readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                फर्काऊ;
            }
            यदि (m_inLogic && m_stmtp) {
                m_stmtp->addNextHere(m_state.createForceRdUpdateStmt(*varInfo));
            }
            फर्काऊ;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        यदि (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            फर्काऊ;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        यदि (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } अन्यथा यदि (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_गाँठो_अभिव्यक्ति* अचर readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_गाँठो_अभिव्यक्ति* अचर exprp = VN_AS(nodep, गाँठो_अभिव्यक्ति);
            Ast_गाँठो* अचर basep = exprp->baseFromp(सत्य);
            AstVarRef* अचर baseRefp = VN_CAST(basep, VarRef);
            यदि (baseRefp) {
                AstVar* अचर varp = baseRefp->varp();
                यदि (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    अचर ForceState::VarForceInfo* अचर varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    यदि (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        अचर पूर्णाङ्क forcePathIndex = varInfo->findForcePathIndex(exprp);
                        यदि (forcePathIndex >= 0) {
                            यदि (!baseRefp->access().isReadOnly()) फर्काऊ;
                            Ast_गाँठो_अभिव्यक्ति* अचर readExprp = m_state.createForceReadIndexExpression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), forcePathIndex));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                            फर्काऊ;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    explicit ForceReplaceVisitor(AstNetlist* nodep, अचर ForceState& state)
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
    यदि (!v3Global.hasForceableSignals()) फर्काऊ;
    ForceState state{असत्य, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_रूख("force", 0, dump_रूख_Either_Level() >= 3);
}

स्थिर शून्य assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    यदि (!v3Global.hasAssignDeassign()) फर्काऊ;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.थप_back(deassignp); });
    लागि (AstDeassign* अचर deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.clear();
    nodep->foreach([&](Ast_गाँठो_Stmt* nodep) {
        यदि (AstAssignCont* अचर assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.थप_back(assignContp);
        } अन्यथा यदि (AstDeassign* अचर deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.थप_back(deassignp);
        }
    });

    लागि (AstAssignCont* अचर assignp : assignContps) {
        assignp->replaceWith(नूतन AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_रूख(), assignp);
    }
    लागि (AstDeassign* अचर deassignp : deassignps) {
        deassignp->replaceWith(
            नूतन AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_रूख_Pure(सत्य)});
        VL_DO_DANGLING(deassignp->delete_रूख(), deassignp);
    }
    ForceState state{सत्य, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_रूख("assign-deassign", 0, dump_रूख_Either_Level() >= 3);
}

शून्य V3Force::forceAndAssignAll(AstNetlist* nodep) {
    अचर VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
