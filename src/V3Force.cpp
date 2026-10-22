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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Force.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी ForceState final {
खुला:
    काष्ठा ForceRange VL_NOT_FINAL {
        पूर्णांक m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        पूर्णांक m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        पूर्णांक m_padLsb = 0;  // Bit positions for RHS padding
        पूर्णांक m_padMsb = 0;  // Bit positions for RHS padding
    };

    काष्ठा ForceInfo final : ForceRange {
        // MEMBERS
        पूर्णांक m_forceId = 0;  // Unique (per signal) variable of this force assignment
        bool m_hasArraySel = false;  // If this has an array select on LHS
        bool m_isExternal = false;  // Synthetic force for the public force controls
        bool m_skipRdRefresh = false;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = nullptr;  // Scope of the var containing RHSID
        Ast_गाँठ_व्यंजक* m_rhsExprp = nullptr;  // Expression on RHS of this force assignment

        ForceInfo() = शेष;
        ForceInfo(पूर्णांक rangeLsb, पूर्णांक rangeMsb, पूर्णांक padLsb, पूर्णांक padMsb, पूर्णांक forceId,
                  bool hasArraySel, AstVarScope* rhsVarVscp, Ast_गाँठ_व्यंजक* rhsExprp)
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

    काष्ठा VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = nullptr;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = nullptr;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = nullptr;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = nullptr;  // Scope of the forced (original) variable
        AstVar* m_varp = nullptr;  // The forced (original) variable
        AstScope* m_scopep = nullptr;  // Scope containing the forced variable
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, पूर्णांक> m_force_Path_To_सूचकांक;
        पूर्णांक m_अग्र_Force_Path_सूचकांक = 1;  // Start at 1 so 0 can be the base path (whole signal)

    निजी:
        अटल व्योम build_Force_Path_कुंजी_Recurse(Ast_गाँठ_व्यंजक* nodep, string& out) {
            अगर (VN_IS(nodep, VarRef)) वापस;
            अगर (स्थिर AstSel* स्थिर selp = VN_CAST(nodep, Sel)) {
                build_Force_Path_कुंजी_Recurse(selp->fromp(), out);
                out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
                वापस;
            }
            अगर (AstStructSel* स्थिर selp = VN_CAST(nodep, StructSel)) {
                Ast_गाँठ_व्यंजक* स्थिर fromp = selp->fromp();
                build_Force_Path_कुंजी_Recurse(fromp, out);
                out += "|T" + selp->name();
                वापस;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        अटल string force_Path_कुंजी(Ast_गाँठ_व्यंजक* nodep) {
            string out;
            build_Force_Path_कुंजी_Recurse(nodep, out);
            वापस out;
        }

    खुला:
        पूर्णांक get_Or_Create_Force_Path_सूचकांक(Ast_गाँठ_व्यंजक* nodep) {
            स्थिर स्वतः pair
                = m_force_Path_To_सूचकांक.emplace(force_Path_कुंजी(nodep), m_अग्र_Force_Path_सूचकांक);
            अगर (pair.second) ++m_अग्र_Force_Path_सूचकांक;
            वापस pair.first->second;
        }

        पूर्णांक find_Force_Path_सूचकांक(Ast_गाँठ_व्यंजक* nodep) स्थिर {
            स्थिर स्वतः it = m_force_Path_To_सूचकांक.find(force_Path_कुंजी(nodep));
            वापस it != m_force_Path_To_सूचकांक.end() ? it->second : -1;
        }
    };

    काष्ठा ForceHelperVars final {
        AstVar* m_rdVarp = nullptr;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = nullptr;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = nullptr;  // The __VforceVal (force value) variable
    };

    काष्ठा ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        bool m_hasBitSel = false;  // Bit-select was found along the path too
    };

    काष्ठा ForceRangeInfo final : ForceRange {
        bool m_hasArraySel = false;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

निजी:
    using ScopeVarCache = std::unordered_map<स्थिर AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser2InUse m_user2InUse;

खुला:
    using ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    using ProtectedVarRefSet = std::unordered_set<स्थिर AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    using BuiltVarScopeSet = std::unordered_set<स्थिर AstVarScope*>;

निजी:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::vector<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, पूर्णांक> m_varToId;
    std::unordered_set<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::vector<ForceInfo*>> m_rhsDepToForces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    bool m_doingAssign = false;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    ProtectedVarRefSet& m_permanentlyProtected;
    BuiltVarScopeSet& m_forceRdUpdateBuilt;

खुला:
    ForceState(bool doingAssign, ForceHelperVarsByVar& forceHelperVarsByVar,
               ProtectedVarRefSet& permanentlyProtected, BuiltVarScopeSet& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doingAssign{doingAssign}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    अटल AstConst* makeZeroConst(Ast_गाँठ* nodep, पूर्णांक width) {
        V3_संख्या zero{nodep, width};
        zero.setAllBits0();
        वापस नया AstConst{nodep->fileline(), zero};
    }

    अटल AstConst* makeConst32(File_पंक्ति* flp, पूर्णांक मूल्य) {
        वापस नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, static_cast<uint32_t>(मूल्य)};
    }

    अटल AstConst* makeRangeMaskConst(Ast_गाँठ* nodep, पूर्णांक width, पूर्णांक lsb, पूर्णांक msb) {
        V3_संख्या mask{nodep, width};
        mask.setAllBits0();
        क्रम (पूर्णांक bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        वापस नया AstConst{nodep->fileline(), mask};
    }

    अटल Ast_गाँठ_व्यंजक* zeroPadToBaseWidth(Ast_गाँठ_व्यंजक* exprp, पूर्णांक baseWidth, पूर्णांक padLsb,
                                           पूर्णांक padMsb) {
        अगर (baseWidth <= 0) वापस exprp;
        स्थिर पूर्णांक निचला_Pad = padLsb;
        स्थिर पूर्णांक ऊपरी_Pad = baseWidth - (padMsb + 1);
        अगर (निचला_Pad > 0) {
            exprp = नया AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, निचला_Pad)};
        }
        अगर (ऊपरी_Pad > 0) {
            exprp = नया AstConcat{exprp->fileline(), makeZeroConst(exprp, ऊपरी_Pad), exprp};
        }
        वापस exprp;
    }

    अटल bool isUnpackedArrayDType(स्थिर Ast_गाँठ_DType* dtypep) {
        वापस VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    अटल bool isBitwiseDType(स्थिर Ast_गाँठ* nodep) {
        स्थिर AstBasicDType* स्थिर basicp = nodep->dtypep()->skipRefp()->basicp();
        वापस basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    अटल Ast_गाँठ_व्यंजक* cast_To_गाँठ_DType(Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ* dtypeFromp) {
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = dtypeFromp->dtypep()->skipRefp();
        स्थिर AstBasicDType* स्थिर basicp = dtypep->basicp();
        अगर (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            वापस exprp;
        }
        वापस नया AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    अटल व्योम markNonReplaceable(AstVarRef* स्थिर nodep) { nodep->user1SetOnce(); }
    bool isNotReplaceable(स्थिर AstVarRef* स्थिर nodep) स्थिर {
        वापस nodep->user1() || m_permanentlyProtected.गिनती(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    व्योम markPermanentlyProtected(AstVarRef* स्थिर nodep) स्थिर {
        markNonReplaceable(nodep);
        m_permanentlyProtected.जोड़ो(nodep);
    }

    अटल std::vector<ForceInfo*> forceInfosInIdOrder(VarForceInfo& info) {
        std::vector<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.आकार());
        क्रम (स्वतः& it : info.m_forces) forceps.धकेलो_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](स्थिर ForceInfo* ap, स्थिर ForceInfo* bp) {
            वापस ap->m_forceId < bp->m_forceId;
        });
        वापस forceps;
    }

    अटल std::vector<स्थिर ForceInfo*> forceInfosInIdOrder(स्थिर VarForceInfo& info) {
        std::vector<स्थिर ForceInfo*> forceps;
        forceps.reserve(info.m_forces.आकार());
        क्रम (स्थिर स्वतः& it : info.m_forces) forceps.धकेलो_back(&it.second);
        std::sort(forceps.begin(), forceps.end(), [](स्थिर ForceInfo* ap, स्थिर ForceInfo* bp) {
            वापस ap->m_forceId < bp->m_forceId;
        });
        वापस forceps;
    }

    अटल bool isOpaquePathSelector(स्थिर Ast_गाँठ* nodep) {
        वापस VN_IS(nodep, Sel) || VN_IS(nodep, गाँठ_Sel) || VN_IS(nodep, StructSel);
    }

    अटल bool isOutermostOpaquePathSelector(स्थिर Ast_गाँठ* nodep) {
        अगर (!isOpaquePathSelector(nodep)) वापस false;
        स्थिर Ast_गाँठ* स्थिर backp = nodep->backp();
        वापस !backp || !isOpaquePathSelector(backp);
    }

    अटल AstVarRef* getOneVarRef(Ast_गाँठ_व्यंजक* forceStmtp) {
        Ast_गाँठ* स्थिर basep = forceStmtp->baseFromp(true);
        अगर (AstSampled* sampledp = VN_CAST(basep, Sampled))
            अगर (Ast_गाँठ_व्यंजक* exprp = VN_CAST(sampledp->exprp(), गाँठ_व्यंजक))
                वापस getOneVarRef(exprp);
        AstVarRef* स्थिर varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        वापस varRefp;
    }

    अटल Ast_गाँठ_व्यंजक* buildNestedArraySel(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* fromp,
                                            स्थिर std::vector<पूर्णांक>& indicies) {
        Ast_गाँठ_व्यंजक* curp = fromp;
        क्रम (स्थिर पूर्णांक idx : indicies) curp = नया AstArraySel{flp, curp, idx};
        वापस curp;
    }

    ढाँचा <typename Fn>
    अटल Ast_गाँठ_Stmt* foreachUnpackedLeaf(स्थिर std::vector<AstUnpackArrayDType*>& dims,
                                            Fn buildLeaf) {
        Ast_गाँठ_Stmt* headp = nullptr;
        Ast_गाँठ_Stmt* tailp = nullptr;
        अगर (dims.empty()) वापस nullptr;
        पूर्णांक योग = 1;
        क्रम (स्थिर AstUnpackArrayDType* स्थिर d : dims) योग *= d->elementsConst();
        अगर (योग <= 0) वापस nullptr;
        std::vector<पूर्णांक> idx(dims.आकार(), 0);
        क्रम (पूर्णांक flat = 0; flat < योग; ++flat) {
            Ast_गाँठ_Stmt* स्थिर stmtp = buildLeaf(idx, flat);
            अगर (!headp) {
                headp = stmtp;
            } अन्यथा {
                tailp->add_अग्र(stmtp);
            }
            tailp = stmtp;
            क्रम (पूर्णांक d = static_cast<पूर्णांक>(dims.आकार()) - 1; d >= 0; --d) {
                अगर (++idx[d] < dims[d]->elementsConst()) अवरोध;
                idx[d] = 0;
            }
        }
        वापस headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_गाँठ_व्यंजक* lhsp, AstVar* varp,
                                     bool requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        अगर (स्थिर AstSel* स्थिर outerSelp = VN_CAST(lhsp, Sel)) {
            पूर्णांक योग_Lsb = 0;
            क्रम (Ast_गाँठ_व्यंजक* curp = lhsp; स्थिर AstSel* स्थिर selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                अगर (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                योग_Lsb += selp->lsbConst();
            }
            info.m_padLsb = योग_Lsb;
            info.m_padMsb = योग_Lsb + outerSelp->widthConst() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        अगर (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::vector<पूर्णांक> indices;
            indices.reserve(info.m_arrayInfo.m_sels.आकार());
            क्रम (AstArraySel* स्थिर selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.धकेलो_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flatten_सूचकांक(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } अन्यथा अगर (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        अगर (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            स्थिर पूर्णांक सूचकांक_2 = varInfo.get_Or_Create_Force_Path_सूचकांक(lhsp);
            info.m_rangeLsb = सूचकांक_2;
            info.m_rangeMsb = सूचकांक_2;
        }
        वापस info;
    }

    Ast_गाँठ_व्यंजक* add_Rhs_मूल्य_Reads(स्थिर VarForceInfo& varInfo, Ast_गाँठ_व्यंजक* exprp) स्थिर {
        अगर (!doingAssign()) वापस exprp;

        स्थिर std::vector<स्थिर ForceInfo*> forceps = forceInfosInIdOrder(varInfo);
        अगर (forceps.empty()) वापस exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* स्थिर cexprp = नया AstCExpr{exprp->fileline(), AstCExpr::Pure{}};
        cexprp->dtypeFrom(exprp);
        cexprp->add("(");
        क्रम (स्थिर ForceInfo* स्थिर finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* स्थिर refp
                = नया AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->add("(void)(");
            cexprp->add(refp);
            cexprp->add("), ");
        }
        cexprp->add(exprp);
        cexprp->add(")");
        वापस cexprp;
    }

    Ast_गाँठ_व्यंजक* createForceReadCall(स्थिर VarForceInfo& varInfo, File_पंक्ति* flp, VCMethod method,
                                     Ast_गाँठ_व्यंजक* originalExprp, Ast_गाँठ* dtypeFromp,
                                     Ast_गाँठ_व्यंजक* सूचकांक_Exprp) स्थिर {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_गाँठ_व्यंजक* स्थिर origValp
            = add_Rhs_मूल्य_Reads(varInfo, cast_To_गाँठ_DType(originalExprp, dtypeFromp));

        AstCMethodHard* स्थिर callp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, method, origValp};
        अगर (सूचकांक_Exprp) callp->addPinsp(सूचकांक_Exprp);
        callp->dtypeFrom(dtypeFromp);
        वापस callp;
    }

    Ast_गाँठ_व्यंजक* createForceRdExpression(स्थिर VarForceInfo& varInfo, File_पंक्ति* flp) स्थिर {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* स्थिर varp = varInfo.m_varVscp->varp();
        अगर (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            अगर (doingAssign()) { वापस नया AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, true);
            AstVarRef* स्थिर baseRefp = नया AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(baseRefp);
            वापस createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, baseRefp, varp,
                                       nullptr);
        }
        AstVarRef* स्थिर baseRefp = नया AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(baseRefp);
        Ast_गाँठ_व्यंजक* स्थिर enRefp = नया AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_गाँठ_व्यंजक* स्थिर valRefp = नया AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        अगर (isBitwiseDType(varp)) {
            वापस नया AstOr{
                flp, नया AstAnd{flp, enRefp, valRefp},
                नया AstAnd{flp, नया AstNot{flp, enRefp->clone_वृक्ष_Pure(false)}, baseRefp}};
        }
        वापस नया AstCond{flp, enRefp, valRefp, baseRefp};
    }

    Ast_गाँठ_Stmt* createForceRdUpdateStmt(स्थिर VarForceInfo& varInfo) स्थिर {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* स्थिर varp = varInfo.m_varVscp->varp();
        अगर (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            वापस createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_पंक्ति* स्थिर flp = varInfo.m_varVscp->fileline();
        वापस नया AstAssign{flp, नया AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_गाँठ_Stmt* createForceRdUpdateStmtUnpacked(स्थिर VarForceInfo& varInfo) स्थिर {
        File_पंक्ति* स्थिर flp = varInfo.m_varVscp->fileline();
        AstVar* स्थिर varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* स्थिर arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        स्थिर std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_गाँठ_Stmt* स्थिर stmtsp = foreachUnpackedLeaf(
            dims, [&](स्थिर std::vector<पूर्णांक>& idx, पूर्णांक /*flat*/) -> Ast_गाँठ_Stmt* {
                AstVarRef* स्थिर baseRefp = नया AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(baseRefp);
                Ast_गाँठ_व्यंजक* स्थिर baseSelp = buildNestedArraySel(flp, baseRefp, idx);
                Ast_गाँठ_व्यंजक* स्थिर enSelp = buildNestedArraySel(
                    flp, नया AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_गाँठ_व्यंजक* स्थिर valSelp = buildNestedArraySel(
                    flp, नया AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_गाँठ_व्यंजक* स्थिर readExprp = नया AstOr{
                    flp, नया AstAnd{flp, enSelp, valSelp},
                    नया AstAnd{flp, नया AstNot{flp, enSelp->clone_वृक्ष_Pure(false)}, baseSelp}};
                Ast_गाँठ_व्यंजक* स्थिर rdLhsSelp = buildNestedArraySel(
                    flp, नया AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                वापस नया AstAssign{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        क्रम (स्थिर ForceInfo* स्थिर finfop : forceInfosInIdOrder(varInfo)) {
            अगर (!finfop->m_isExternal) जारी;
            stmtsp->add_अग्र(नया AstAssign{flp,
                                          नया AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_वृक्ष_Pure(false)});
        }
        वापस stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        स्थिर स्वतः it = m_varToId.find(vscp);
        अगर (it != m_varToId.end()) वापस m_varInfos[it->second];

        m_varToId.emplace(vscp, m_varInfos.आकार());
        m_varInfos.emplace_back();
        VarForceInfo& info = m_varInfos.back();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* स्थिर varp = info.m_varp;
        अगर (!varp->isForceable()) वापस info;

        File_पंक्ति* स्थिर flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        स्थिर bool helperVarsBuilt = helperVars.m_rdVarp != nullptr;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != nullptr)
                        && helperVarsBuilt == (helperVars.m_valVarp != nullptr),
                    varp, "Incomplete force helper set");
        अगर (!helperVarsBuilt) {
            स्थिर bool unpacked = isUnpackedArrayDType(varp->dtypep());
            स्थिर VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_गाँठ_DType* स्थिर enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = नया AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sigPublic(true);
            helperVars.m_enVarp
                = नया AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(true);
            helperVars.m_valVarp
                = नया AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(true);
            varp->add_अग्र_Here(helperVars.m_rdVarp);
            varp->add_अग्र_Here(helperVars.m_enVarp);
            varp->add_अग्र_Here(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = findScopeVar(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = findScopeVar(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = findScopeVar(info.m_scopep, helperVars.m_valVarp);
        अगर (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } अन्यथा {
            info.m_forceRdVscp = नया AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = नया AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = नया AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->addVarsp(info.m_forceRdVscp);
            info.m_scopep->addVarsp(info.m_forceEnVscp);
            info.m_scopep->addVarsp(info.m_forceValVscp);
        }
        वापस info;
    }

    व्योम markClockedWrite(AstVar* varp) { m_clockedWrites.जोड़ो(varp); }
    bool hasClockedWrite(AstVar* varp) स्थिर { वापस m_clockedWrites.गिनती(varp); }

    bool doingAssign() स्थिर { वापस m_doingAssign; }

    स्थिर VarForceInfo* getVarInfo(AstVarScope* vscp) स्थिर {
        स्थिर स्वतः it = m_varToId.find(vscp);
        वापस it != m_varToId.end() ? &m_varInfos[it->second] : nullptr;
    }

    AstVarScope* findScopeVar(AstScope* scopep, स्थिर AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        अगर (cache.empty()) {
            क्रम (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        स्थिर स्वतः it = cache.find(varp);
        वापस it != cache.end() ? it->second : nullptr;
    }
    व्योम addForceAssignment(AstVar* varp, AstVarScope* vscp, Ast_गाँठ_व्यंजक* rhsExprp,
                            AstAssignForce* forceStmtp, पूर्णांक rangeLsb, पूर्णांक rangeMsb, पूर्णांक padLsb,
                            पूर्णांक padMsb, bool hasArraySel, bool skipRdRefresh = false) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        स्थिर पूर्णांक forceId = info.m_forces.आकार();
        File_पंक्ति* स्थिर flp = varp->fileline();
        AstScope* स्थिर scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        अगर (!info.m_forceVecVscp) {
            AstCDType* स्थिर forceVecDtypep = नया AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* स्थिर forceVecVarp
                = नया AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doingAssign ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(false);
            forceVecVarp->isInternal(true);
            varp->add_अग्र_Here(forceVecVarp);
            info.m_forceVecVscp = नया AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        स्वतः pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, nullptr, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        अगर (doingAssign()) {
            std::vector<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* स्थिर refp) {
                अगर (!refp->access().isReadOnly()) वापस;
                AstVar* स्थिर depVarp = refp->varp();
                अगर (depVarp
                    && std::find(depVarps.begin(), depVarps.end(), depVarp) == depVarps.end()) {
                    depVarps.धकेलो_back(depVarp);
                }
            });
            क्रम (AstVar* स्थिर depVarp : depVarps) m_rhsDepToForces[depVarp].धकेलो_back(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    अटल व्योम collectArraySelInfo(Ast_गाँठ_व्यंजक* exprp, ArraySelInfo& info) {
        अगर (स्थिर स्वतः* स्थिर selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = true;
            collectArraySelInfo(selp->fromp(), info);
        } अन्यथा अगर (स्वतः* स्थिर selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.धकेलो_back(selp);
        } अन्यथा अगर (स्थिर स्वतः* स्थिर memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    अटल ArraySelInfo getArraySelInfo(Ast_गाँठ_व्यंजक* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        वापस info;
    }

    अटल std::vector<पूर्णांक> arraySelDimSizes(स्थिर ArraySelInfo& info) {
        std::vector<पूर्णांक> dims;
        dims.reserve(info.m_sels.आकार());
        क्रम (AstArraySel* स्थिर selp : info.m_sels) {
            Ast_गाँठ_DType* स्थिर dtypep = selp->fromp()->dtypep()->skipRefp();
            स्थिर AstUnpackArrayDType* स्थिर arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.धकेलो_back(arrayp->declRange().elements());
        }
        वापस dims;
    }

    अटल पूर्णांक flatten_सूचकांक(स्थिर std::vector<पूर्णांक>& indices, स्थिर std::vector<पूर्णांक>& dimSizes) {
        UASSERT(indices.आकार() == dimSizes.आकार(), "Array index and dimension size mismatch");
        पूर्णांक सूचकांक_2 = 0;
        पूर्णांक stride = 1;
        क्रम (पूर्णांक i = static_cast<पूर्णांक>(indices.आकार()) - 1; i >= 0; --i) {
            सूचकांक_2 += indices[i] * stride;
            stride *= dimSizes[i];
        }
        वापस सूचकांक_2;
    }

    अटल Ast_गाँठ_व्यंजक* build_Flatten_सूचकांक_व्यंजक(File_पंक्ति* flp, स्थिर ArraySelInfo& info) {
        स्थिर std::vector<पूर्णांक> dimSizes = arraySelDimSizes(info);
        bool allConst = true;
        क्रम (AstArraySel* स्थिर selp : info.m_sels) {
            अगर (!VN_IS(selp->bitp(), Const)) {
                allConst = false;
                अवरोध;
            }
        }
        अगर (allConst) {
            std::vector<पूर्णांक> constIndices;
            constIndices.reserve(info.m_sels.आकार());
            क्रम (AstArraySel* स्थिर selp : info.m_sels) {
                constIndices.धकेलो_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            वापस makeConst32(flp, flatten_सूचकांक(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_गाँठ_व्यंजक* resultp = nullptr;
        पूर्णांक stride = 1;
        क्रम (पूर्णांक i = static_cast<पूर्णांक>(info.m_sels.आकार()) - 1; i >= 0; --i) {
            Ast_गाँठ_व्यंजक* termp = info.m_sels[i]->bitp()->clone_वृक्ष_Pure(false);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            अगर (termp->width() < 32) termp = नया AstExtend{flp, termp, 32};
            अगर (stride != 1) termp = नया AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? नया AstAdd{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        वापस resultp;
    }

    अटल Ast_गाँठ_व्यंजक* build_Rhs_तथ्य_व्यंजक(File_पंक्ति* flp, स्थिर ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        वापस नया AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    व्योम finalizeRhsVars() {
        क्रम (VarForceInfo& info : m_varInfos) {
            AstVar* स्थिर varp = info.m_varp;
            अगर (info.m_forces.empty()) जारी;

            AstScope* स्थिर scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_पंक्ति* स्थिर flp = varp->fileline();
            स्थिर std::vector<ForceInfo*> forceps = forceInfosInIdOrder(info);

            क्रम (ForceInfo* स्थिर finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* स्थिर rhsVarp = नया AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doingAssign() ? "_VassignRHS" : "__VforceRHS")
                        + std::to_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(true);
                rhsVarp->sigPublic(true);
                rhsVarp->setForcedByCode();
                varp->add_अग्र_Here(rhsVarp);
                finfo.m_rhsVarVscp = नया AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                // end
                AstAssign* स्थिर rhsAssignp = नया AstAssign{
                    flp, नया AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                अगर (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* स्थिर touchCallp = नया AstCMethodHard{
                        flp, नया AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtypeSetVoid();
                    Ast_गाँठ_Stmt* स्थिर touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->add_अग्र_Here(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* स्थिर alwaysp
                    = नया AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                Ast_Sen_वृक्ष* स्थिर senTreep
                    = नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* स्थिर activep = नया AstActive{flp, "force-rhs-update", senTreep};
                activep->sen_वृक्ष_Storep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }

            अगर (info.m_forceRdVscp) {
                अगर (!m_forceRdUpdateBuilt.जोड़ो(info.m_varVscp).second) जारी;
                AstActive* स्थिर activeInitp = नया AstActive{
                    flp, "force-init",
                    नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Static{}}}};
                activeInitp->sen_वृक्ष_Storep(activeInitp->sentreep());
                Ast_गाँठ_Stmt* initStmtp = nullptr;
                अगर (AstUnpackArrayDType* स्थिर arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    स्थिर std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    स्थिर पूर्णांक innerWidth = dims.back()->subDTypep()->skipRefp()->width();
                    initStmtp = foreachUnpackedLeaf(
                        dims, [&](स्थिर std::vector<पूर्णांक>& idx, पूर्णांक /*flat*/) -> Ast_गाँठ_Stmt* {
                            Ast_गाँठ_व्यंजक* स्थिर lhsp = buildNestedArraySel(
                                flp, नया AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            वापस नया AstAssign{flp, lhsp, makeZeroConst(varp, innerWidth)};
                        });
                } अन्यथा {
                    initStmtp = नया AstAssign{
                        flp, नया AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->width())};
                }
                initStmtp->add_अग्र(createForceRdUpdateStmt(info));
                activeInitp->addStmtsp(नया AstInitial{flp, initStmtp});
                scopep->addBlocksp(activeInitp);

                AstSenItem* itemsp = nullptr;
                स्वतः addSenItem = [&](AstVarScope* vscp) {
                    अगर (!vscp) वापस;
                    AstSenItem* स्थिर nextp = नया AstSenItem{
                        flp, VEdgeType::ET_CHANGED, नया AstVarRef{flp, vscp, VAccess::READ}};
                    अगर (itemsp) {
                        itemsp->add_अग्र(nextp);
                    } अन्यथा {
                        itemsp = nextp;
                    }
                };
                addSenItem(info.m_forceEnVscp);
                addSenItem(info.m_forceValVscp);
                AstVarRef* स्थिर origSenRefp = नया AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* स्थिर origItemp
                    = नया AstSenItem{flp, VEdgeType::ET_CHANGED, origSenRefp};
                अगर (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->add_अग्र(origItemp);
                क्रम (ForceInfo* स्थिर finfop : forceps) {
                    अगर (!finfop->m_isExternal) addSenItem(finfop->m_rhsVarVscp);
                }

                AstActive* स्थिर activep
                    = नया AstActive{flp, "force-rd-update", नया Ast_Sen_वृक्ष{flp, itemsp}};
                activep->sen_वृक्ष_Storep(activep->sentreep());
                activep->addStmtsp(नया AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr,
                                                 createForceRdUpdateStmt(info)});
                scopep->addBlocksp(activep);
            }
        }
    }

    Ast_गाँठ* createRhsUpdatesForWrite(File_पंक्ति* flp, AstVar* writtenVarp) स्थिर {
        अगर (!doingAssign()) वापस nullptr;

        स्थिर स्वतः it = m_rhsDepToForces.find(writtenVarp);
        अगर (it == m_rhsDepToForces.end()) वापस nullptr;

        Ast_गाँठ* headp = nullptr;
        Ast_गाँठ* tailp = nullptr;
        क्रम (स्थिर ForceInfo* स्थिर finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            AstAssign* स्थिर updatep
                = नया AstAssign{flp, नया AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_वृक्ष_Pure(false)};
            अगर (tailp) {
                tailp->add_अग्र_Here(updatep);
            } अन्यथा {
                headp = updatep;
            }
            tailp = updatep;
        }
        वापस headp;
    }

    स्थिर ForceInfo& getForceInfo(AstAssignForce* forceStmtp) स्थिर {
        AstVarScope* स्थिर vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        स्थिर VarForceInfo* स्थिर varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        स्वतः it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.end(), "Force statement not found");
        वापस it2->second;
    }

    अटल bool selOverlapsAnyForce(स्थिर VarForceInfo& varInfo, पूर्णांक selLsb, पूर्णांक selMsb) {
        क्रम (स्थिर स्वतः& pair : varInfo.m_forces) {
            अगर (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) वापस true;
        }
        वापस false;
    }

    Ast_गाँठ_व्यंजक* createForceReadExpression(स्थिर VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) स्थिर {
        File_पंक्ति* स्थिर flp = originalRefp->fileline();
        AstVarRef* स्थिर refp = originalRefp->clone_वृक्ष_Pure(false);
        refp->access(VAccess::READ);
        वापस createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   nullptr);
    }

    Ast_गाँठ_व्यंजक* create_Force_Read_सूचकांक_Expression(स्थिर VarForceInfo& varInfo,
                                                Ast_गाँठ_व्यंजक* originalExprp,
                                                Ast_गाँठ_व्यंजक* सूचकांक_Exprp) स्थिर {
        File_पंक्ति* स्थिर flp = originalExprp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर exprp = originalExprp->clone_वृक्ष_Pure(false);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        वापस createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_सूचकांक, exprp, originalExprp,
                                   सूचकांक_Exprp);
    }

    अटल Ast_गाँठ_व्यंजक* rebuildSelPath(Ast_गाँठ_व्यंजक* pathp, Ast_गाँठ_व्यंजक* baseExprp) {
        अगर (स्थिर AstSel* स्थिर selp = VN_CAST(pathp, Sel)) {
            Ast_गाँठ_व्यंजक* स्थिर fromp = rebuildSelPath(selp->fromp(), baseExprp);
            AstSel* स्थिर outp
                = नया AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->widthConst()};
            outp->dtypeFrom(selp);
            वापस outp;
        }
        वापस baseExprp;
    }
};

// Split deassign concat LHS before converting to release internals.
अटल व्योम splitDeassign(AstDeassign* nodep) {
    AstConcat* स्थिर concatp = VN_CAST(nodep->lhsp(), Concat);
    अगर (!concatp) वापस;

    File_पंक्ति* स्थिर flp = nodep->fileline();
    AstDeassign* स्थिर newLp = नया AstDeassign{flp, concatp->lhsp()->unlinkFrBack()};
    AstDeassign* स्थिर newRp = नया AstDeassign{flp, concatp->rhsp()->unlinkFrBack()};
    Ast_गाँठ_व्यंजक* स्थिर conp = concatp->unlinkFrBack();
    nodep->replaceWith(newLp);
    newLp->add_अग्र_Here(newRp);
    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    VL_DO_DANGLING(conp->delete_वृक्ष(), conp);

    splitDeassign(newLp);
    splitDeassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

श्रेणी ForceDiscoveryVisitor final : खुला VNVisitorConst {
    ForceState& m_state;
    bool m_inClockedActive = false;

    व्योम buildForceableUnpackedArray(AstVarScope* स्थिर nodep,
                                     AstUnpackArrayDType* स्थिर arrDtypep) {
        AstVar* स्थिर varp = nodep->varp();
        स्थिर std::vector<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.empty(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        स्थिर Ast_गाँठ_DType* स्थिर leafDtypep = dims.back()->subDTypep()->skipRefp();
        स्थिर AstBasicDType* स्थिर innerBasicp = leafDtypep->basicp();
        अगर (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            वापस;
        }

        File_पंक्ति* स्थिर flp = varp->fileline();
        स्थिर पूर्णांक innerWidth = leafDtypep->width();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* स्थिर enVscp = info.m_forceEnVscp;
        AstVarScope* स्थिर valVscp = info.m_forceValVscp;

        AstSenItem* स्थिर itemsp = नया AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  नया AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* स्थिर activep = नया AstActive{flp, "force-update", नया Ast_Sen_वृक्ष{flp, itemsp}};
        activep->sen_वृक्ष_Storep(activep->sentreep());

        Ast_गाँठ_Stmt* स्थिर alwaysBodyHeadp = ForceState::foreachUnpackedLeaf(
            dims, [&](स्थिर std::vector<पूर्णांक>& idx, पूर्णांक flat) -> Ast_गाँठ_Stmt* {
                AstVarRef* स्थिर origRefp = नया AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_गाँठ_व्यंजक* स्थिर origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_गाँठ_व्यंजक* स्थिर enSelp = ForceState::buildNestedArraySel(
                    flp, नया AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_गाँठ_व्यंजक* स्थिर valSelp = ForceState::buildNestedArraySel(
                    flp, नया AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_गाँठ_व्यंजक* स्थिर forceExprp = नया AstOr{
                    flp, नया AstAnd{flp, enSelp, valSelp},
                    नया AstAnd{flp, नया AstNot{flp, enSelp->clone_वृक्ष_Pure(false)}, origSelp}};
                Ast_गाँठ_व्यंजक* स्थिर lhsSelp = ForceState::buildNestedArraySel(
                    flp, नया AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                AstAssignForce* स्थिर forceAssignp = नया AstAssignForce{flp, lhsSelp, forceExprp};
                forceAssignp->user2(true);

                Ast_गाँठ_व्यंजक* स्थिर rhsClonep = forceExprp->clone_वृक्ष_Pure(false);
                rhsClonep->foreach([varp](AstVarRef* स्थिर r) {
                    अगर (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/innerWidth - 1,
                                           /*hasArraySel=*/true, /*skipRdRefresh=*/true);
                वापस forceAssignp;
            });
        activep->addStmtsp(नया AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, alwaysBodyHeadp});
        nodep->scopep()->addBlocksp(activep);
    }

    व्योम visit(AstAssignForce* nodep) override {
        अगर (nodep->user2()) वापस;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* स्थिर lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* स्थिर forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, true);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_गाँठ_व्यंजक* स्थिर rhsExprp = nodep->rhsp()->clone_वृक्ष_Pure(false);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    व्योम visit(AstAssign* nodep) override {
        अगर (m_state.doingAssign() && m_inClockedActive) {
            अगर (AstVarRef* स्थिर lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstActive* nodep) override {
        VL_RESTORER(m_inClockedActive);
        m_inClockedActive = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstVarScope* nodep) override {
        अगर (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            अगर (m_state.doingAssign()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                वापस;
            }

            अगर (AstUnpackArrayDType* स्थिर arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                वापस;
            }

            स्थिर AstBasicDType* स्थिर bdtypep = nodep->varp()->basicp();
            अगर (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* स्थिर varp = nodep->varp();
            File_पंक्ति* स्थिर flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* स्थिर enVscp = info.m_forceEnVscp;
            AstVarScope* स्थिर valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            AstSenItem* स्थिर itemsp = नया AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                      नया AstVarRef{flp, enVscp, VAccess::READ}};
            AstActive* स्थिर activep
                = नया AstActive{flp, "force-update", नया Ast_Sen_वृक्ष{flp, itemsp}};
            activep->sen_वृक्ष_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* स्थिर refp = नया AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* स्थिर enRefp = नया AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* स्थिर valRefp = नया AstVarRef{flp, valVscp, VAccess::READ};
            स्थिर AstBasicDType* स्थिर basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_गाँठ_व्यंजक* स्थिर forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstOr{
                            flp, नया AstAnd{flp, enRefp, valRefp},
                            नया AstAnd{flp, नया AstNot{flp, enRefp->clone_वृक्ष_Pure(false)}, refp}})
                      : static_cast<Ast_गाँठ_व्यंजक*>(नया AstCond{flp, enRefp, valRefp, refp});
            AstAssignForce* स्थिर forceAssignp
                = नया AstAssignForce{flp, नया AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(true);
            activep->addStmtsp(नया AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
            nodep->scopep()->addBlocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_गाँठ_व्यंजक* स्थिर rhsClonep = forceExprp->clone_वृक्ष_Pure(false);
            rhsClonep->foreach([varp](AstVarRef* स्थिर refp) {
                अगर (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            स्थिर bool bitwiseVar = ForceState::isBitwiseDType(varp);
            स्थिर पूर्णांक padMsb = bitwiseVar ? (varp->width() - 1) : 0;
            पूर्णांक rangeLsb = 0;
            पूर्णांक rangeMsb = padMsb;
            अगर (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.addForceAssignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, false);
        }
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_अग्र_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

श्रेणी ForceConvertVisitor final : खुला VNVisitor {
    ForceState& m_state;

    व्योम visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
        AstVarRef* स्थिर lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* स्थिर forcedVarp = lhsVarRefp->varp();

        स्थिर ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        स्थिर ForceState::VarForceInfo* स्थिर varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_पंक्ति* स्थिर flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* स्थिर rhsAssignp
            = नया AstAssign{flp, नया AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_वृक्ष_Pure(false)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        स्थिर bool bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        अगर (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_गाँठ_व्यंजक* baseExprp = nodep->rhsp()->clone_वृक्ष_Pure(false);
            baseExprp->foreach(
                [](AstVarRef* स्थिर refp) { ForceState::markNonReplaceable(refp); });
            अगर (bitwiseForcedVar) {
                baseExprp = ForceState::zeroPadToBaseWidth(baseExprp, forcedVarp->width(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            अगर (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* स्थिर maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_गाँठ_व्यंजक* स्थिर valReadp
                    = नया AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_गाँठ_व्यंजक* स्थिर valWritep
                    = नया AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_गाँठ_व्यंजक* स्थिर notMaskp = नया AstNot{flp, maskConstp};
                Ast_गाँठ_व्यंजक* स्थिर maskedOldp = नया AstAnd{flp, valReadp, notMaskp};
                Ast_गाँठ_व्यंजक* स्थिर newValp = नया AstOr{flp, maskedOldp, baseExprp};
                valAssignp = नया AstAssign{flp, valWritep, newValp};

                Ast_गाँठ_व्यंजक* स्थिर enReadp
                    = नया AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_गाँठ_व्यंजक* स्थिर enWritep
                    = नया AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_गाँठ_व्यंजक* स्थिर newEnp
                    = नया AstOr{flp, enReadp, maskConstp->clone_वृक्ष_Pure(false)};
                enAssignp = नया AstAssign{flp, enWritep, newEnp};
            } अन्यथा {
                AstConst* स्थिर oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                Ast_गाँठ_व्यंजक* स्थिर rhsValp = ForceState::cast_To_गाँठ_DType(baseExprp, forcedVarp);
                valAssignp = नया AstAssign{
                    flp, नया AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = नया AstAssign{
                    flp, नया AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        स्थिर AstSel* स्थिर selLhsp = VN_CAST(lhsp, Sel);
        स्थिर bool arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->width();
        Ast_गाँठ_व्यंजक* स्थिर rhsDatap = ForceState::build_Rhs_तथ्य_व्यंजक(flp, info);
        AstCExpr* स्थिर rhsAddrp = नया AstCExpr{flp};
        rhsAddrp->add("&(");
        rhsAddrp->add(rhsDatap);
        rhsAddrp->add(")");
        AstCMethodHard* स्थिर addForceCallp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_ADD,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        addForceCallp->addPinsp(rhsAddrp);
        addForceCallp->addPinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        अगर (arrayBitSel) {
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padLsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, info.m_padMsb));
            addForceCallp->addPinsp(ForceState::makeConst32(flp, selLhsp->fromp()->width()));
        }
        addForceCallp->dtypeSetVoid();
        Ast_गाँठ_Stmt* स्थिर stmtp = addForceCallp->makeStmt();

        Ast_गाँठ* tailp = rhsAssignp;
        अगर (valAssignp) {
            tailp->add_अग्र_Here(valAssignp);
            tailp = valAssignp;
        }
        अगर (enAssignp) {
            tailp->add_अग्र_Here(enAssignp);
            tailp = enAssignp;
        }
        tailp->add_अग्र_Here(stmtp);
        अगर (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->add_अग्र_Here(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
        AstVarRef* स्थिर lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* स्थिर releasedVarp = lhsVarRefp->varp();

        स्थिर ForceState::VarForceInfo* स्थिर varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        अगर (!varInfo) {
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        File_पंक्ति* स्थिर flp = nodep->fileline();

        स्थिर ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, false);

        स्थिर AstSel* स्थिर selLhsp = VN_CAST(lhsp, Sel);
        स्थिर bool arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->width();
        AstCMethodHard* स्थिर releaseCallp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        अगर (arrayBitSel) {
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->addPinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtypeSetVoid();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_गाँठ_Stmt* स्थिर releasep = releaseCallp->makeStmt();

        AstAssign* clearEnp = nullptr;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        अगर (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_गाँठ_व्यंजक* स्थिर enWritep
                = नया AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            अगर (ForceState::isBitwiseDType(releasedVarp)) {
                स्थिर पूर्णांक varWidth = releasedVarp->width();
                अगर (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1) {
                    clearEnp
                        = नया AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, varWidth)};
                } अन्यथा {
                    // forceEn = forceEn & ~mask(range);
                    Ast_गाँठ_व्यंजक* स्थिर enReadp
                        = नया AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* स्थिर maskConst = ForceState::makeRangeMaskConst(
                        nodep, varWidth, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_गाँठ_व्यंजक* स्थिर newEnp
                        = नया AstAnd{flp, enReadp, नया AstNot{flp, maskConst}};
                    clearEnp = नया AstAssign{flp, enWritep, newEnp};
                }
            } अन्यथा {
                clearEnp = नया AstAssign{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        स्थिर AstSel* स्थिर selp = VN_CAST(lhsp, Sel);
        Ast_गाँठ_व्यंजक* स्थिर basep = selp ? selp->fromp() : lhsp;

        Ast_गाँठ* stmtListp = releasep;
        अगर (clearEnp) {
            clearEnp->add_अग्र_Here(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        स्थिर bool fullBitwiseRelease
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->width() - 1;
        अगर (!releasedVarp->isContinuously()
            && !(m_state.doingAssign() && m_state.hasClockedWrite(releasedVarp)
                 && fullBitwiseRelease)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            स्थिर bool hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_गाँठ_व्यंजक* forceReadp = nullptr;
            अगर (hasOpaquePath) {
                forceReadp = m_state.create_Force_Read_सूचकांक_Expression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } अन्यथा अगर (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_सूचकांक_Expression(
                    *varInfo, basep,
                    ForceState::build_Flatten_सूचकांक_व्यंजक(flp, rangeInfo.m_arrayInfo));
                अगर (selp) { forceReadp = ForceState::rebuildSelPath(lhsp, forceReadp); }
            } अन्यथा {
                forceReadp
                    = selp ? ForceState::rebuildSelPath(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            AstAssign* स्थिर assignp = नया AstAssign{flp, lhsp->clone_वृक्ष_Pure(false), forceReadp};
            assignp->add_अग्र_Here(stmtListp);
            stmtListp = assignp;
        }

        अगर (varInfo->m_forceRdVscp) stmtListp->add_अग्र(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_अग्र_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

श्रेणी ForceReplaceVisitor final : खुला VNVisitor {
    स्थिर ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_गाँठ_Stmt* m_stmtp = nullptr;
    bool m_inLogic = false;

    व्योम iterateLogic(Ast_गाँठ* nodep) {
        VL_RESTORER(m_inLogic);
        m_inLogic = true;
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstAssign* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        अगर (AstVarRef* स्थिर lhsp = VN_CAST(nodep->lhsp()->baseFromp(true), VarRef)) {
            अगर (Ast_गाँठ* स्थिर updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->add_अग्र_Here(updatep);
            }
        }
    }
    व्योम visit(AstAssignCont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_अग्र_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    व्योम visit(AstDeassign*) override {}
    व्योम visit(AstCFunc* nodep) override { iterateLogic(nodep); }
    व्योम visit(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override { iterateLogic(nodep); }
    व्योम visit(AstAlways* nodep) override {
        अगर (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            वापस;
        }
        iterateLogic(nodep);
    }
    व्योम visit(AstSenItem* nodep) override { iterateLogic(nodep); }
    व्योम visit(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* स्थिर refp = VN_CAST(nodep->fromp(), VarRef);
        अगर (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            visit(static_cast<Ast_गाँठ*>(nodep));
            वापस;
        }

        AstVar* स्थिर varp = refp->varp();
        स्थिर ForceState::VarForceInfo* स्थिर varInfo = m_state.getVarInfo(refp->varScopep());
        अगर (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.empty()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            visit(static_cast<Ast_गाँठ*>(nodep));
            वापस;
        }

        अगर (स्थिर AstConst* स्थिर lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            स्थिर पूर्णांक selLsb = lsbConstp->toSInt();
            स्थिर पूर्णांक selMsb = selLsb + nodep->width() - 1;
            अगर (!varp->isSigPublic()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                visit(static_cast<Ast_गाँठ*>(nodep));
                वापस;
            }
        }

        File_पंक्ति* स्थिर flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* स्थिर refClonep = refp->clone_वृक्ष_Pure(false);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* स्थिर callp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->width())};
        callp->addPinsp(refClonep);
        callp->addPinsp(nodep->lsbp()->clone_वृक्ष_Pure(false));
        callp->addPinsp(ForceState::makeConst32(flp, nodep->width()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstArraySel* nodep) override {
        अगर (स्थिर AstArraySel* स्थिर backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            अगर (backSelp->fromp() == nodep) {
                iterateChildren(nodep);
                वापस;
            }
        }

        Ast_गाँठ* स्थिर basep = nodep->baseFromp(true);
        AstVarRef* स्थिर baseRefp = VN_CAST(basep, VarRef);
        अगर (!baseRefp) {
            iterateChildren(nodep);
            वापस;
        }
        AstVar* स्थिर varp = baseRefp->varp();
        स्थिर ForceState::VarForceInfo* स्थिर varInfo = m_state.getVarInfo(baseRefp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        अगर (m_state.isNotReplaceable(baseRefp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            वापस;
        }

        अगर (!baseRefp->access().isReadOnly()) {
            iterateChildren(nodep);
            वापस;
        }
        अगर (m_state.doingAssign() && varInfo->m_forceRdVscp) {
            baseRefp->varp(varInfo->m_forceRdVscp->varp());
            baseRefp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            वापस;
        }
        स्थिर ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        क्रम (AstArraySel* स्थिर selp : arrayInfo.m_sels) iterate_And_अग्र_Null(selp->bitp());
        Ast_गाँठ_व्यंजक* स्थिर सूचकांक_Exprp
            = ForceState::build_Flatten_सूचकांक_व्यंजक(nodep->fileline(), arrayInfo);
        Ast_गाँठ_व्यंजक* स्थिर readExprp
            = m_state.create_Force_Read_सूचकांक_Expression(*varInfo, nodep, सूचकांक_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(AstVarRef* nodep) override {
        अगर (m_state.isNotReplaceable(nodep)) वापस;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        अगर (स्थिर AstArraySel* स्थिर backSelp = VN_CAST(nodep->backp(), ArraySel)) {
            अगर (backSelp->fromp() == nodep) वापस;
        }

        स्थिर ForceState::VarForceInfo* स्थिर varInfo = m_state.getVarInfo(nodep->varScopep());
        अगर (!varInfo) वापस;

        अगर (varInfo->m_forceRdVscp) {
            अगर (nodep->access().isRW()) {
                अगर (m_inLogic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                वापस;
            }
            अगर (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_गाँठ_व्यंजक* स्थिर readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            }
            अगर (m_inLogic && m_stmtp) {
                m_stmtp->add_अग्र_Here(m_state.createForceRdUpdateStmt(*varInfo));
            }
            वापस;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        अगर (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            वापस;
        }

        UASSERT_OBJ(!varInfo->m_forces.empty(), nodep, "Var info for variable with no forces");

        अगर (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } अन्यथा अगर (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_गाँठ_व्यंजक* स्थिर readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (ForceState::isOutermostOpaquePathSelector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_गाँठ_व्यंजक* स्थिर exprp = VN_AS(nodep, गाँठ_व्यंजक);
            Ast_गाँठ* स्थिर basep = exprp->baseFromp(true);
            AstVarRef* स्थिर baseRefp = VN_CAST(basep, VarRef);
            अगर (baseRefp) {
                AstVar* स्थिर varp = baseRefp->varp();
                अगर (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    स्थिर ForceState::VarForceInfo* स्थिर varInfo
                        = m_state.getVarInfo(baseRefp->varScopep());
                    अगर (!m_state.isNotReplaceable(baseRefp) && varInfo) {
                        स्थिर पूर्णांक force_Path_सूचकांक = varInfo->find_Force_Path_सूचकांक(exprp);
                        अगर (force_Path_सूचकांक >= 0) {
                            अगर (!baseRefp->access().isReadOnly()) वापस;
                            Ast_गाँठ_व्यंजक* स्थिर readExprp = m_state.create_Force_Read_सूचकांक_Expression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_Path_सूचकांक));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                            वापस;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

खुला:
    explicit ForceReplaceVisitor(AstNetlist* nodep, स्थिर ForceState& state)
        : m_state{state} {
        iterate_And_अग्र_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::addStat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

अटल व्योम forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::ProtectedVarRefSet& permanentlyProtected,
                         ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    अगर (!v3Global.hasForceableSignals()) वापस;
    ForceState state{false, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्ष("force", 0, dump_वृक्ष_Either_Level() >= 3);
}

अटल व्योम assignAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::ProtectedVarRefSet& permanentlyProtected,
                          ForceState::BuiltVarScopeSet& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    अगर (!v3Global.hasAssignDeassign()) वापस;

    std::vector<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.धकेलो_back(deassignp); });
    क्रम (AstDeassign* स्थिर deassignp : deassignps) splitDeassign(deassignp);

    std::vector<AstAssignCont*> assignContps;
    deassignps.सफा();
    nodep->foreach([&](Ast_गाँठ_Stmt* nodep) {
        अगर (AstAssignCont* स्थिर assignContp = VN_CAST(nodep, AssignCont)) {
            assignContps.धकेलो_back(assignContp);
        } अन्यथा अगर (AstDeassign* स्थिर deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.धकेलो_back(deassignp);
        }
    });

    क्रम (AstAssignCont* स्थिर assignp : assignContps) {
        assignp->replaceWith(नया AstAssignForce{assignp->fileline(),
                                                assignp->lhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
        VL_DO_DANGLING(assignp->delete_वृक्ष(), assignp);
    }
    क्रम (AstDeassign* स्थिर deassignp : deassignps) {
        deassignp->replaceWith(
            नया AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_वृक्ष_Pure(true)});
        VL_DO_DANGLING(deassignp->delete_वृक्ष(), deassignp);
    }
    ForceState state{true, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्ष("assign-deassign", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3Force::forceAndAssignAll(AstNetlist* nodep) {
    स्थिर VNUser3InUse user3InUse;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::ProtectedVarRefSet permanentlyProtected;
    ForceState::BuiltVarScopeSet forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    assignAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
