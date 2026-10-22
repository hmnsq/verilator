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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Force.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి ForceState final {
బహిరంగ:
    నిర్మాణం ForceRange VL_NOT_FINAL {
        పూర్ణాంకం m_rangeLsb = 0;  // VlForceVec range: bit index or array element index
        పూర్ణాంకం m_rangeMsb = 0;  // VlForceVec range: bit index or array element index
        పూర్ణాంకం m_padLsb = 0;  // Bit positions for RHS padding
        పూర్ణాంకం m_padMsb = 0;  // Bit positions for RHS padding
    };

    నిర్మాణం ForceInfo final : ForceRange {
        // MEMBERS
        పూర్ణాంకం m_forceId = 0;  // Unique (per signal) variable of this force assignment
        తార్కిక m_hasArraySel = అబద్ధం;  // If this has an array select on LHS
        తార్కిక m_isExternal = అబద్ధం;  // Synthetic force for the public force controls
        తార్కిక m_skipRdRefresh = అబద్ధం;  // finalizeRhsVars's force-rd-update block refreshes forceRd
        AstVarScope* m_rhsVarVscp = శూన్యసూచిక;  // Scope of the var containing RHSID
        Ast_కణుపు_సమాసం* m_rhsExprp = శూన్యసూచిక;  // Expression on RHS of this force assignment

        ForceInfo() = అప్రమేయ;
        ForceInfo(పూర్ణాంకం rangeLsb, పూర్ణాంకం rangeMsb, పూర్ణాంకం padLsb, పూర్ణాంకం padMsb, పూర్ణాంకం forceId,
                  తార్కిక hasArraySel, AstVarScope* rhsVarVscp, Ast_కణుపు_సమాసం* rhsExprp)
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

    నిర్మాణం VarForceInfo final {
        AstVarScope* m_forceVecVscp = శూన్యసూచిక;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = శూన్యసూచిక;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = శూన్యసూచిక;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = శూన్యసూచిక;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = శూన్యసూచిక;  // Scope of the forced (original) variable
        AstVar* m_varp = శూన్యసూచిక;  // The forced (original) variable
        AstScope* m_scopep = శూన్యసూచిక;  // Scope containing the forced variable
        std::unordered_map<Ast_కేటాయించు_Force*, ForceInfo> m_forces;
        std::unordered_map<string, పూర్ణాంకం> m_force_మార్గం_గమ్యం_సూచీ;
        పూర్ణాంకం m_తదుపరి_Force_మార్గం_సూచీ = 1;  // Start at 1 so 0 can be the base path (whole signal)

    అంతరంగ:
        స్థిర శూన్య build_Force_మార్గం_కీలకం_Recurse(Ast_కణుపు_సమాసం* nodep, string& బహిర్గతం) {
            ఒకవేళ (VN_IS(nodep, VarRef)) తిరిగి;
            ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
                build_Force_మార్గం_కీలకం_Recurse(selp->fromp(), బహిర్గతం);
                బహిర్గతం += "|S" + cvt_గమ్యం_Str(selp->lsbConst()) + ":" + cvt_గమ్యం_Str(selp->వెడల్పు_Const());
                తిరిగి;
            }
            ఒకవేళ (AstStructSel* స్థిరరాశి selp = VN_CAST(nodep, StructSel)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి fromp = selp->fromp();
                build_Force_మార్గం_కీలకం_Recurse(fromp, బహిర్గతం);
                బహిర్గతం += "|T" + selp->name();
                తిరిగి;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        స్థిర string force_మార్గం_కీలకం(Ast_కణుపు_సమాసం* nodep) {
            string బహిర్గతం;
            build_Force_మార్గం_కీలకం_Recurse(nodep, బహిర్గతం);
            తిరిగి బహిర్గతం;
        }

    బహిరంగ:
        పూర్ణాంకం get_Or_Create_Force_మార్గం_సూచీ(Ast_కణుపు_సమాసం* nodep) {
            స్థిరరాశి స్వయంచాలక pair
                = m_force_మార్గం_గమ్యం_సూచీ.emplace(force_మార్గం_కీలకం(nodep), m_తదుపరి_Force_మార్గం_సూచీ);
            ఒకవేళ (pair.రెండవ) ++m_తదుపరి_Force_మార్గం_సూచీ;
            తిరిగి pair.మొదటి->రెండవ;
        }

        పూర్ణాంకం కనుగొను_Force_మార్గం_సూచీ(Ast_కణుపు_సమాసం* nodep) స్థిరరాశి {
            స్థిరరాశి స్వయంచాలక it = m_force_మార్గం_గమ్యం_సూచీ.కనుగొను(force_మార్గం_కీలకం(nodep));
            తిరిగి it != m_force_మార్గం_గమ్యం_సూచీ.చివర() ? it->రెండవ : -1;
        }
    };

    నిర్మాణం ForceHelperVars final {
        AstVar* m_rdVarp = శూన్యసూచిక;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = శూన్యసూచిక;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = శూన్యసూచిక;  // The __VforceVal (force value) variable
    };

    నిర్మాణం ArraySelInfo final {
        std::సదిశ<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        తార్కిక m_has_బిట్టు_Sel = అబద్ధం;  // Bit-select was found along the path too
    };

    నిర్మాణం ForceRangeInfo final : ForceRange {
        తార్కిక m_hasArraySel = అబద్ధం;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

అంతరంగ:
    ఉపయోగించి ScopeVarCache = std::unordered_map<స్థిరరాశి AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;

బహిరంగ:
    ఉపయోగించి ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    ఉపయోగించి Protected_Var_Ref_సమితి = std::unordered_సమితి<స్థిరరాశి AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    ఉపయోగించి Built_Var_Scope_సమితి = std::unordered_సమితి<స్థిరరాశి AstVarScope*>;

అంతరంగ:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::సదిశ<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, పూర్ణాంకం> m_var_గమ్యం_Id;
    std::unordered_సమితి<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::సదిశ<ForceInfo*>> m_rhs_Dep_గమ్యం_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    తార్కిక m_doing_కేటాయించు = అబద్ధం;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_సమితి& m_permanentlyProtected;
    Built_Var_Scope_సమితి& m_forceRdUpdateBuilt;

బహిరంగ:
    ForceState(తార్కిక doing_కేటాయించు, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_సమితి& permanentlyProtected, Built_Var_Scope_సమితి& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_కేటాయించు{doing_కేటాయించు}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    స్థిర AstConst* makeZeroConst(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు) {
        V3_సంఖ్య zero{nodep, వెడల్పు};
        zero.సమితి_All_Bits0();
        తిరిగి కొత్త AstConst{nodep->fileline(), zero};
    }

    స్థిర AstConst* makeConst32(File_పంక్తి* flp, పూర్ణాంకం విలువ) {
        తిరిగి కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, static_cast<uint32_t>(విలువ)};
    }

    స్థిర AstConst* make_Range_ముసుగు_Const(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు, పూర్ణాంకం lsb, పూర్ణాంకం msb) {
        V3_సంఖ్య ముసుగు{nodep, వెడల్పు};
        ముసుగు.సమితి_All_Bits0();
        కొరకు (పూర్ణాంకం బిట్టు_2 = lsb; బిట్టు_2 <= msb; ++బిట్టు_2) ముసుగు.సమితి_బిట్టు(బిట్టు_2, 1);
        తిరిగి కొత్త AstConst{nodep->fileline(), ముసుగు};
    }

    స్థిర Ast_కణుపు_సమాసం* zero_Pad_గమ్యం_పునాది_వెడల్పు(Ast_కణుపు_సమాసం* exprp, పూర్ణాంకం పునాది_వెడల్పు, పూర్ణాంకం padLsb,
                                           పూర్ణాంకం padMsb) {
        ఒకవేళ (పునాది_వెడల్పు <= 0) తిరిగి exprp;
        స్థిరరాశి పూర్ణాంకం దిగువ_Pad = padLsb;
        స్థిరరాశి పూర్ణాంకం ఎగువ_Pad = పునాది_వెడల్పు - (padMsb + 1);
        ఒకవేళ (దిగువ_Pad > 0) {
            exprp = కొత్త AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, దిగువ_Pad)};
        }
        ఒకవేళ (ఎగువ_Pad > 0) {
            exprp = కొత్త AstConcat{exprp->fileline(), makeZeroConst(exprp, ఎగువ_Pad), exprp};
        }
        తిరిగి exprp;
    }

    స్థిర తార్కిక isUnpackedArrayDType(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        తిరిగి VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    స్థిర తార్కిక isBitwiseDType(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = nodep->dtypep()->skipRefp()->basicp();
        తిరిగి basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    స్థిర Ast_కణుపు_సమాసం* cast_గమ్యం_కణుపు_DType(Ast_కణుపు_సమాసం* exprp, Ast_కణుపు* dtypeFromp) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = dtypeFromp->dtypep()->skipRefp();
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp();
        ఒకవేళ (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            తిరిగి exprp;
        }
        తిరిగి కొత్త AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    స్థిర శూన్య markNonReplaceable(AstVarRef* స్థిరరాశి nodep) { nodep->user1_సమితి_Once(); }
    తార్కిక isNotReplaceable(స్థిరరాశి AstVarRef* స్థిరరాశి nodep) స్థిరరాశి {
        తిరిగి nodep->user1() || m_permanentlyProtected.లెక్క(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    శూన్య markPermanentlyProtected(AstVarRef* స్థిరరాశి nodep) స్థిరరాశి {
        markNonReplaceable(nodep);
        m_permanentlyProtected.చొప్పించు(nodep);
    }

    స్థిర std::సదిశ<ForceInfo*> force_Infos_లోపల_Id_క్రమం(VarForceInfo& info) {
        std::సదిశ<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.కొలత());
        కొరకు (స్వయంచాలక& it : info.m_forces) forceps.నెట్టు_వెనుక(&it.రెండవ);
        std::క్రమీకరించు(forceps.begin(), forceps.చివర(), [](స్థిరరాశి ForceInfo* ap, స్థిరరాశి ForceInfo* bp) {
            తిరిగి ap->m_forceId < bp->m_forceId;
        });
        తిరిగి forceps;
    }

    స్థిర std::సదిశ<స్థిరరాశి ForceInfo*> force_Infos_లోపల_Id_క్రమం(స్థిరరాశి VarForceInfo& info) {
        std::సదిశ<స్థిరరాశి ForceInfo*> forceps;
        forceps.reserve(info.m_forces.కొలత());
        కొరకు (స్థిరరాశి స్వయంచాలక& it : info.m_forces) forceps.నెట్టు_వెనుక(&it.రెండవ);
        std::క్రమీకరించు(forceps.begin(), forceps.చివర(), [](స్థిరరాశి ForceInfo* ap, స్థిరరాశి ForceInfo* bp) {
            తిరిగి ap->m_forceId < bp->m_forceId;
        });
        తిరిగి forceps;
    }

    స్థిర తార్కిక is_Opaque_మార్గం_Selector(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి VN_IS(nodep, Sel) || VN_IS(nodep, కణుపు_Sel) || VN_IS(nodep, StructSel);
    }

    స్థిర తార్కిక is_Outermost_Opaque_మార్గం_Selector(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (!is_Opaque_మార్గం_Selector(nodep)) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();
        తిరిగి !backp || !is_Opaque_మార్గం_Selector(backp);
    }

    స్థిర AstVarRef* getOneVarRef(Ast_కణుపు_సమాసం* forceStmtp) {
        Ast_కణుపు* స్థిరరాశి basep = forceStmtp->పునాది_Fromp(నిజం);
        ఒకవేళ (AstSampled* sampledp = VN_CAST(basep, Sampled))
            ఒకవేళ (Ast_కణుపు_సమాసం* exprp = VN_CAST(sampledp->exprp(), కణుపు_సమాసం))
                తిరిగి getOneVarRef(exprp);
        AstVarRef* స్థిరరాశి varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        తిరిగి varRefp;
    }

    స్థిర Ast_కణుపు_సమాసం* buildNestedArraySel(File_పంక్తి* flp, Ast_కణుపు_సమాసం* fromp,
                                            స్థిరరాశి std::సదిశ<పూర్ణాంకం>& indicies) {
        Ast_కణుపు_సమాసం* curp = fromp;
        కొరకు (స్థిరరాశి పూర్ణాంకం idx : indicies) curp = కొత్త AstArraySel{flp, curp, idx};
        తిరిగి curp;
    }

    మూస <రకం_పేరు Fn>
    స్థిర Ast_కణుపు_Stmt* foreach_Unpacked_ఆకు(స్థిరరాశి std::సదిశ<AstUnpackArrayDType*>& dims,
                                            Fn build_ఆకు) {
        Ast_కణుపు_Stmt* headp = శూన్యసూచిక;
        Ast_కణుపు_Stmt* tailp = శూన్యసూచిక;
        ఒకవేళ (dims.ఖాళీలు()) తిరిగి శూన్యసూచిక;
        పూర్ణాంకం మొత్తం = 1;
        కొరకు (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి d : dims) మొత్తం *= d->elementsConst();
        ఒకవేళ (మొత్తం <= 0) తిరిగి శూన్యసూచిక;
        std::సదిశ<పూర్ణాంకం> idx(dims.కొలత(), 0);
        కొరకు (పూర్ణాంకం flat = 0; flat < మొత్తం; ++flat) {
            Ast_కణుపు_Stmt* స్థిరరాశి stmtp = build_ఆకు(idx, flat);
            ఒకవేళ (!headp) {
                headp = stmtp;
            } లేకపోతే {
                tailp->కూడు_తదుపరి(stmtp);
            }
            tailp = stmtp;
            కొరకు (పూర్ణాంకం d = static_cast<పూర్ణాంకం>(dims.కొలత()) - 1; d >= 0; --d) {
                ఒకవేళ (++idx[d] < dims[d]->elementsConst()) ఆపు;
                idx[d] = 0;
            }
        }
        తిరిగి headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_కణుపు_సమాసం* lhsp, AstVar* varp,
                                     తార్కిక requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.ఖాళీలు();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->వెడల్పు() - 1) : 0;

        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి outerSelp = VN_CAST(lhsp, Sel)) {
            పూర్ణాంకం మొత్తం_Lsb = 0;
            కొరకు (Ast_కణుపు_సమాసం* curp = lhsp; స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                ఒకవేళ (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                మొత్తం_Lsb += selp->lsbConst();
            }
            info.m_padLsb = మొత్తం_Lsb;
            info.m_padMsb = మొత్తం_Lsb + outerSelp->వెడల్పు_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        ఒకవేళ (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::సదిశ<పూర్ణాంకం> indices;
            indices.reserve(info.m_arrayInfo.m_sels.కొలత());
            కొరకు (AstArraySel* స్థిరరాశి selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.నెట్టు_వెనుక(VN_AS(selp->bitp(), Const)->గమ్యం_SInt());
            }
            info.m_rangeLsb = flatten_సూచీ(indices, arraySelDimSizes(info.m_arrayInfo));

            info.m_rangeMsb = info.m_rangeLsb;
        } లేకపోతే ఒకవేళ (isUnpackedArrayDType(varp->dtypep())) {
            lhsp->v3fatalSrc("Whole unpacked-array force/release should have been lowered via "
                             "element selections");
        }
        ఒకవేళ (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            // Non-bitwise member/struct paths cannot use a real bit range, so map each distinct
            // source path onto a synthetic index in VlForceVec and use that index consistently
            // for force, release, and readback.
            VarForceInfo& varInfo = getOrCreateVarInfo(getOneVarRef(lhsp)->varScopep());
            స్థిరరాశి పూర్ణాంకం సూచీ = varInfo.get_Or_Create_Force_మార్గం_సూచీ(lhsp);
            info.m_rangeLsb = సూచీ;
            info.m_rangeMsb = సూచీ;
        }
        తిరిగి info;
    }

    Ast_కణుపు_సమాసం* కూడు_Rhs_విలువ_Reads(స్థిరరాశి VarForceInfo& varInfo, Ast_కణుపు_సమాసం* exprp) స్థిరరాశి {
        ఒకవేళ (!doing_కేటాయించు()) తిరిగి exprp;

        స్థిరరాశి std::సదిశ<స్థిరరాశి ForceInfo*> forceps = force_Infos_లోపల_Id_క్రమం(varInfo);
        ఒకవేళ (forceps.ఖాళీలు()) తిరిగి exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* స్థిరరాశి cexprp = కొత్త AstCExpr{exprp->fileline(), AstCExpr::శుద్ధ{}};
        cexprp->dtypeFrom(exprp);
        cexprp->కూడు("(");
        కొరకు (స్థిరరాశి ForceInfo* స్థిరరాశి finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* స్థిరరాశి refp
                = కొత్త AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->కూడు("(void)(");
            cexprp->కూడు(refp);
            cexprp->కూడు("), ");
        }
        cexprp->కూడు(exprp);
        cexprp->కూడు(")");
        తిరిగి cexprp;
    }

    Ast_కణుపు_సమాసం* createForceReadCall(స్థిరరాశి VarForceInfo& varInfo, File_పంక్తి* flp, VCMethod పద్ధతి,
                                     Ast_కణుపు_సమాసం* originalExprp, Ast_కణుపు* dtypeFromp,
                                     Ast_కణుపు_సమాసం* సూచీ_Exprp) స్థిరరాశి {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_కణుపు_సమాసం* స్థిరరాశి origValp
            = కూడు_Rhs_విలువ_Reads(varInfo, cast_గమ్యం_కణుపు_DType(originalExprp, dtypeFromp));

        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, పద్ధతి, origValp};
        ఒకవేళ (సూచీ_Exprp) callp->కూడు_Pinsp(సూచీ_Exprp);
        callp->dtypeFrom(dtypeFromp);
        తిరిగి callp;
    }

    Ast_కణుపు_సమాసం* create_Force_Rd_వ్యక్తీకరణ(స్థిరరాశి VarForceInfo& varInfo, File_పంక్తి* flp) స్థిరరాశి {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* స్థిరరాశి varp = varInfo.m_varVscp->varp();
        ఒకవేళ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            ఒకవేళ (doing_కేటాయించు()) { తిరిగి కొత్త AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNOPTFLAT, నిజం);
            AstVarRef* స్థిరరాశి పునాది_Refp = కొత్త AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(పునాది_Refp);
            తిరిగి createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, పునాది_Refp, varp,
                                       శూన్యసూచిక);
        }
        AstVarRef* స్థిరరాశి పునాది_Refp = కొత్త AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(పునాది_Refp);
        Ast_కణుపు_సమాసం* స్థిరరాశి enRefp = కొత్త AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_కణుపు_సమాసం* స్థిరరాశి valRefp = కొత్త AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        ఒకవేళ (isBitwiseDType(varp)) {
            తిరిగి కొత్త AstOr{
                flp, కొత్త AstAnd{flp, enRefp, valRefp},
                కొత్త AstAnd{flp, కొత్త AstNot{flp, enRefp->clone_వృక్షం_శుద్ధ(అబద్ధం)}, పునాది_Refp}};
        }
        తిరిగి కొత్త AstCond{flp, enRefp, valRefp, పునాది_Refp};
    }

    Ast_కణుపు_Stmt* createForceRdUpdateStmt(స్థిరరాశి VarForceInfo& varInfo) స్థిరరాశి {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* స్థిరరాశి varp = varInfo.m_varVscp->varp();
        ఒకవేళ (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            తిరిగి createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_పంక్తి* స్థిరరాశి flp = varInfo.m_varVscp->fileline();
        తిరిగి కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             create_Force_Rd_వ్యక్తీకరణ(varInfo, flp)};
    }

    Ast_కణుపు_Stmt* createForceRdUpdateStmtUnpacked(స్థిరరాశి VarForceInfo& varInfo) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి flp = varInfo.m_varVscp->fileline();
        AstVar* స్థిరరాశి varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* స్థిరరాశి arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        స్థిరరాశి std::సదిశ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_కణుపు_Stmt* స్థిరరాశి stmtsp = foreach_Unpacked_ఆకు(
            dims, [&](స్థిరరాశి std::సదిశ<పూర్ణాంకం>& idx, పూర్ణాంకం /*flat*/) -> Ast_కణుపు_Stmt* {
                AstVarRef* స్థిరరాశి పునాది_Refp = కొత్త AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(పునాది_Refp);
                Ast_కణుపు_సమాసం* స్థిరరాశి పునాది_Selp = buildNestedArraySel(flp, పునాది_Refp, idx);
                Ast_కణుపు_సమాసం* స్థిరరాశి enSelp = buildNestedArraySel(
                    flp, కొత్త AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_కణుపు_సమాసం* స్థిరరాశి valSelp = buildNestedArraySel(
                    flp, కొత్త AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_కణుపు_సమాసం* స్థిరరాశి readExprp = కొత్త AstOr{
                    flp, కొత్త AstAnd{flp, enSelp, valSelp},
                    కొత్త AstAnd{flp, కొత్త AstNot{flp, enSelp->clone_వృక్షం_శుద్ధ(అబద్ధం)}, పునాది_Selp}};
                Ast_కణుపు_సమాసం* స్థిరరాశి rdLhsSelp = buildNestedArraySel(
                    flp, కొత్త AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                తిరిగి కొత్త Ast_కేటాయించు{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        కొరకు (స్థిరరాశి ForceInfo* స్థిరరాశి finfop : force_Infos_లోపల_Id_క్రమం(varInfo)) {
            ఒకవేళ (!finfop->m_isExternal) కొనసాగు;
            stmtsp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{flp,
                                          కొత్త AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)});
        }
        తిరిగి stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        స్థిరరాశి స్వయంచాలక it = m_var_గమ్యం_Id.కనుగొను(vscp);
        ఒకవేళ (it != m_var_గమ్యం_Id.చివర()) తిరిగి m_varInfos[it->రెండవ];

        m_var_గమ్యం_Id.emplace(vscp, m_varInfos.కొలత());
        m_varInfos.emplace_వెనుక();
        VarForceInfo& info = m_varInfos.వెనుక();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* స్థిరరాశి varp = info.m_varp;
        ఒకవేళ (!varp->isForceable()) తిరిగి info;

        File_పంక్తి* స్థిరరాశి flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        స్థిరరాశి తార్కిక helperVarsBuilt = helperVars.m_rdVarp != శూన్యసూచిక;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != శూన్యసూచిక)
                        && helperVarsBuilt == (helperVars.m_valVarp != శూన్యసూచిక),
                    varp, "Incomplete force helper set");
        ఒకవేళ (!helperVarsBuilt) {
            స్థిరరాశి తార్కిక unpacked = isUnpackedArrayDType(varp->dtypep());
            స్థిరరాశి VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_కణుపు_DType* స్థిరరాశి enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->కనుగొను_బిట్టు_DType();
            helperVars.m_rdVarp
                = కొత్త AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_ప్రజా(నిజం);
            helperVars.m_enVarp
                = కొత్త AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(నిజం);
            helperVars.m_valVarp
                = కొత్త AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(నిజం);
            varp->కూడు_తదుపరి_ఇక్కడ(helperVars.m_rdVarp);
            varp->కూడు_తదుపరి_ఇక్కడ(helperVars.m_enVarp);
            varp->కూడు_తదుపరి_ఇక్కడ(helperVars.m_valVarp);
        }

        info.m_forceRdVscp = కనుగొను_Scope_Var(info.m_scopep, helperVars.m_rdVarp);
        info.m_forceEnVscp = కనుగొను_Scope_Var(info.m_scopep, helperVars.m_enVarp);
        info.m_forceValVscp = కనుగొను_Scope_Var(info.m_scopep, helperVars.m_valVarp);
        ఒకవేళ (info.m_forceRdVscp || info.m_forceEnVscp || info.m_forceValVscp) {
            UASSERT_OBJ(info.m_forceRdVscp && info.m_forceEnVscp && info.m_forceValVscp, vscp,
                        "Incomplete pre-existing force helper set");
        } లేకపోతే {
            info.m_forceRdVscp = కొత్త AstVarScope{flp, info.m_scopep, helperVars.m_rdVarp};
            info.m_forceEnVscp = కొత్త AstVarScope{flp, info.m_scopep, helperVars.m_enVarp};
            info.m_forceValVscp = కొత్త AstVarScope{flp, info.m_scopep, helperVars.m_valVarp};
            info.m_scopep->కూడు_Varsp(info.m_forceRdVscp);
            info.m_scopep->కూడు_Varsp(info.m_forceEnVscp);
            info.m_scopep->కూడు_Varsp(info.m_forceValVscp);
        }
        తిరిగి info;
    }

    శూన్య markClockedWrite(AstVar* varp) { m_clockedWrites.చొప్పించు(varp); }
    తార్కిక hasClockedWrite(AstVar* varp) స్థిరరాశి { తిరిగి m_clockedWrites.లెక్క(varp); }

    తార్కిక doing_కేటాయించు() స్థిరరాశి { తిరిగి m_doing_కేటాయించు; }

    స్థిరరాశి VarForceInfo* getVarInfo(AstVarScope* vscp) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక it = m_var_గమ్యం_Id.కనుగొను(vscp);
        తిరిగి it != m_var_గమ్యం_Id.చివర() ? &m_varInfos[it->రెండవ] : శూన్యసూచిక;
    }

    AstVarScope* కనుగొను_Scope_Var(AstScope* scopep, స్థిరరాశి AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        ఒకవేళ (cache.ఖాళీలు()) {
            కొరకు (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        స్థిరరాశి స్వయంచాలక it = cache.కనుగొను(varp);
        తిరిగి it != cache.చివర() ? it->రెండవ : శూన్యసూచిక;
    }
    శూన్య కూడు_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_కణుపు_సమాసం* rhsExprp,
                            Ast_కేటాయించు_Force* forceStmtp, పూర్ణాంకం rangeLsb, పూర్ణాంకం rangeMsb, పూర్ణాంకం padLsb,
                            పూర్ణాంకం padMsb, తార్కిక hasArraySel, తార్కిక skipRdRefresh = అబద్ధం) {
        v3Global.సమితి_ఉపయోగిస్తుంది_Force();
        varp->సమితి_Forced_By_సంకేతం();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        స్థిరరాశి పూర్ణాంకం forceId = info.m_forces.కొలత();
        File_పంక్తి* స్థిరరాశి flp = varp->fileline();
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        ఒకవేళ (!info.m_forceVecVscp) {
            AstCDType* స్థిరరాశి forceVecDtypep = కొత్త AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(forceVecDtypep);

            AstVar* స్థిరరాశి forceVecVarp
                = కొత్త AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_కేటాయించు ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(అబద్ధం);
            forceVecVarp->is_అంతర్గత(నిజం);
            varp->కూడు_తదుపరి_ఇక్కడ(forceVecVarp);
            info.m_forceVecVscp = కొత్త AstVarScope{flp, scopep, forceVecVarp};
            scopep->కూడు_Varsp(info.m_forceVecVscp);
        }

        స్వయంచాలక pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, శూన్యసూచిక, rhsExprp});
        ForceInfo& finfo = pair.మొదటి->రెండవ;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        ఒకవేళ (doing_కేటాయించు()) {
            std::సదిశ<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* స్థిరరాశి refp) {
                ఒకవేళ (!refp->access().isReadOnly()) తిరిగి;
                AstVar* స్థిరరాశి depVarp = refp->varp();
                ఒకవేళ (depVarp
                    && std::కనుగొను(depVarps.begin(), depVarps.చివర(), depVarp) == depVarps.చివర()) {
                    depVarps.నెట్టు_వెనుక(depVarp);
                }
            });
            కొరకు (AstVar* స్థిరరాశి depVarp : depVarps) m_rhs_Dep_గమ్యం_Forces[depVarp].నెట్టు_వెనుక(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    స్థిర శూన్య collectArraySelInfo(Ast_కణుపు_సమాసం* exprp, ArraySelInfo& info) {
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి selp = VN_CAST(exprp, Sel)) {
            info.m_has_బిట్టు_Sel = నిజం;
            collectArraySelInfo(selp->fromp(), info);
        } లేకపోతే ఒకవేళ (స్వయంచాలక* స్థిరరాశి selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.నెట్టు_వెనుక(selp);
        } లేకపోతే ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    స్థిర ArraySelInfo getArraySelInfo(Ast_కణుపు_సమాసం* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        తిరిగి info;
    }

    స్థిర std::సదిశ<పూర్ణాంకం> arraySelDimSizes(స్థిరరాశి ArraySelInfo& info) {
        std::సదిశ<పూర్ణాంకం> dims;
        dims.reserve(info.m_sels.కొలత());
        కొరకు (AstArraySel* స్థిరరాశి selp : info.m_sels) {
            Ast_కణుపు_DType* స్థిరరాశి dtypep = selp->fromp()->dtypep()->skipRefp();
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.నెట్టు_వెనుక(arrayp->declRange().elements());
        }
        తిరిగి dims;
    }

    స్థిర పూర్ణాంకం flatten_సూచీ(స్థిరరాశి std::సదిశ<పూర్ణాంకం>& indices, స్థిరరాశి std::సదిశ<పూర్ణాంకం>& dimSizes) {
        UASSERT(indices.కొలత() == dimSizes.కొలత(), "Array index and dimension size mismatch");
        పూర్ణాంకం సూచీ = 0;
        పూర్ణాంకం stride = 1;
        కొరకు (పూర్ణాంకం i = static_cast<పూర్ణాంకం>(indices.కొలత()) - 1; i >= 0; --i) {
            సూచీ += indices[i] * stride;
            stride *= dimSizes[i];
        }
        తిరిగి సూచీ;
    }

    స్థిర Ast_కణుపు_సమాసం* build_Flatten_సూచీ_సమాసం(File_పంక్తి* flp, స్థిరరాశి ArraySelInfo& info) {
        స్థిరరాశి std::సదిశ<పూర్ణాంకం> dimSizes = arraySelDimSizes(info);
        తార్కిక allConst = నిజం;
        కొరకు (AstArraySel* స్థిరరాశి selp : info.m_sels) {
            ఒకవేళ (!VN_IS(selp->bitp(), Const)) {
                allConst = అబద్ధం;
                ఆపు;
            }
        }
        ఒకవేళ (allConst) {
            std::సదిశ<పూర్ణాంకం> constIndices;
            constIndices.reserve(info.m_sels.కొలత());
            కొరకు (AstArraySel* స్థిరరాశి selp : info.m_sels) {
                constIndices.నెట్టు_వెనుక(VN_AS(selp->bitp(), Const)->గమ్యం_SInt());
            }
            తిరిగి makeConst32(flp, flatten_సూచీ(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
        పూర్ణాంకం stride = 1;
        కొరకు (పూర్ణాంకం i = static_cast<పూర్ణాంకం>(info.m_sels.కొలత()) - 1; i >= 0; --i) {
            Ast_కణుపు_సమాసం* termp = info.m_sels[i]->bitp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            ఒకవేళ (termp->వెడల్పు() < 32) termp = కొత్త AstExtend{flp, termp, 32};
            ఒకవేళ (stride != 1) termp = కొత్త AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? కొత్త Ast_కూడు{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        తిరిగి resultp;
    }

    స్థిర Ast_కణుపు_సమాసం* build_Rhs_దత్తాంశం_సమాసం(File_పంక్తి* flp, స్థిరరాశి ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        తిరిగి కొత్త AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    శూన్య finalizeRhsVars() {
        కొరకు (VarForceInfo& info : m_varInfos) {
            AstVar* స్థిరరాశి varp = info.m_varp;
            ఒకవేళ (info.m_forces.ఖాళీలు()) కొనసాగు;

            AstScope* స్థిరరాశి scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_పంక్తి* స్థిరరాశి flp = varp->fileline();
            స్థిరరాశి std::సదిశ<ForceInfo*> forceps = force_Infos_లోపల_Id_క్రమం(info);

            కొరకు (ForceInfo* స్థిరరాశి finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* స్థిరరాశి rhsVarp = కొత్త AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_కేటాయించు() ? "_VassignRHS" : "__VforceRHS")
                        + std::గమ్యం_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(నిజం);
                rhsVarp->sig_ప్రజా(నిజం);
                rhsVarp->సమితి_Forced_By_సంకేతం();
                varp->కూడు_తదుపరి_ఇక్కడ(rhsVarp);
                finfo.m_rhsVarVscp = కొత్త AstVarScope{flp, scopep, rhsVarp};
                scopep->కూడు_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //చివర
                Ast_కేటాయించు* స్థిరరాశి rhsAssignp = కొత్త Ast_కేటాయించు{
                    flp, కొత్త AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                ఒకవేళ (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* స్థిరరాశి touchCallp = కొత్త AstCMethodHard{
                        flp, కొత్త AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_సమితి_Void();
                    Ast_కణుపు_Stmt* స్థిరరాశి touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->కూడు_తదుపరి_ఇక్కడ(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* స్థిరరాశి alwaysp
                    = కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, శూన్యసూచిక, rhsAssignp};
                Ast_Sen_వృక్షం* స్థిరరాశి senTreep
                    = కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Combo{}}};
                Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "force-rhs-update", senTreep};
                activep->sen_వృక్షం_Storep(activep->sentreep());
                activep->కూడు_Stmtsp(alwaysp);
                scopep->కూడు_Blocksp(activep);
            }

            ఒకవేళ (info.m_forceRdVscp) {
                ఒకవేళ (!m_forceRdUpdateBuilt.చొప్పించు(info.m_varVscp).రెండవ) కొనసాగు;
                Ast_చురుకైన* స్థిరరాశి చురుకైన_Initp = కొత్త Ast_చురుకైన{
                    flp, "force-init",
                    కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Static{}}}};
                చురుకైన_Initp->sen_వృక్షం_Storep(చురుకైన_Initp->sentreep());
                Ast_కణుపు_Stmt* initStmtp = శూన్యసూచిక;
                ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    స్థిరరాశి std::సదిశ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    స్థిరరాశి పూర్ణాంకం inner_వెడల్పు = dims.వెనుక()->subDTypep()->skipRefp()->వెడల్పు();
                    initStmtp = foreach_Unpacked_ఆకు(
                        dims, [&](స్థిరరాశి std::సదిశ<పూర్ణాంకం>& idx, పూర్ణాంకం /*flat*/) -> Ast_కణుపు_Stmt* {
                            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = buildNestedArraySel(
                                flp, కొత్త AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            తిరిగి కొత్త Ast_కేటాయించు{flp, lhsp, makeZeroConst(varp, inner_వెడల్పు)};
                        });
                } లేకపోతే {
                    initStmtp = కొత్త Ast_కేటాయించు{
                        flp, కొత్త AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->వెడల్పు())};
                }
                initStmtp->కూడు_తదుపరి(createForceRdUpdateStmt(info));
                చురుకైన_Initp->కూడు_Stmtsp(కొత్త AstInitial{flp, initStmtp});
                scopep->కూడు_Blocksp(చురుకైన_Initp);

                Ast_Sen_అంశం* itemsp = శూన్యసూచిక;
                స్వయంచాలక కూడు_Sen_అంశం = [&](AstVarScope* vscp) {
                    ఒకవేళ (!vscp) తిరిగి;
                    Ast_Sen_అంశం* స్థిరరాశి nextp = కొత్త Ast_Sen_అంశం{
                        flp, VEdgeType::ET_మారింది, కొత్త AstVarRef{flp, vscp, VAccess::READ}};
                    ఒకవేళ (itemsp) {
                        itemsp->కూడు_తదుపరి(nextp);
                    } లేకపోతే {
                        itemsp = nextp;
                    }
                };
                కూడు_Sen_అంశం(info.m_forceEnVscp);
                కూడు_Sen_అంశం(info.m_forceValVscp);
                AstVarRef* స్థిరరాశి origSenRefp = కొత్త AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                Ast_Sen_అంశం* స్థిరరాశి origItemp
                    = కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_మారింది, origSenRefp};
                ఒకవేళ (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->కూడు_తదుపరి(origItemp);
                కొరకు (ForceInfo* స్థిరరాశి finfop : forceps) {
                    ఒకవేళ (!finfop->m_isExternal) కూడు_Sen_అంశం(finfop->m_rhsVarVscp);
                }

                Ast_చురుకైన* స్థిరరాశి activep
                    = కొత్త Ast_చురుకైన{flp, "force-rd-update", కొత్త Ast_Sen_వృక్షం{flp, itemsp}};
                activep->sen_వృక్షం_Storep(activep->sentreep());
                activep->కూడు_Stmtsp(కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, శూన్యసూచిక,
                                                 createForceRdUpdateStmt(info)});
                scopep->కూడు_Blocksp(activep);
            }
        }
    }

    Ast_కణుపు* createRhsUpdatesForWrite(File_పంక్తి* flp, AstVar* writtenVarp) స్థిరరాశి {
        ఒకవేళ (!doing_కేటాయించు()) తిరిగి శూన్యసూచిక;

        స్థిరరాశి స్వయంచాలక it = m_rhs_Dep_గమ్యం_Forces.కనుగొను(writtenVarp);
        ఒకవేళ (it == m_rhs_Dep_గమ్యం_Forces.చివర()) తిరిగి శూన్యసూచిక;

        Ast_కణుపు* headp = శూన్యసూచిక;
        Ast_కణుపు* tailp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి ForceInfo* స్థిరరాశి finfop : it->రెండవ) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_కేటాయించు* స్థిరరాశి updatep
                = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            ఒకవేళ (tailp) {
                tailp->కూడు_తదుపరి_ఇక్కడ(updatep);
            } లేకపోతే {
                headp = updatep;
            }
            tailp = updatep;
        }
        తిరిగి headp;
    }

    స్థిరరాశి ForceInfo& getForceInfo(Ast_కేటాయించు_Force* forceStmtp) స్థిరరాశి {
        AstVarScope* స్థిరరాశి vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        స్థిరరాశి VarForceInfo* స్థిరరాశి varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        స్వయంచాలక it2 = varInfo->m_forces.కనుగొను(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.చివర(), "Force statement not found");
        తిరిగి it2->రెండవ;
    }

    స్థిర తార్కిక selOverlapsAnyForce(స్థిరరాశి VarForceInfo& varInfo, పూర్ణాంకం selLsb, పూర్ణాంకం selMsb) {
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : varInfo.m_forces) {
            ఒకవేళ (pair.రెండవ.m_rangeLsb <= selMsb && pair.రెండవ.m_rangeMsb >= selLsb) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    Ast_కణుపు_సమాసం* create_Force_Read_వ్యక్తీకరణ(స్థిరరాశి VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి flp = originalRefp->fileline();
        AstVarRef* స్థిరరాశి refp = originalRefp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        refp->access(VAccess::READ);
        తిరిగి createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   శూన్యసూచిక);
    }

    Ast_కణుపు_సమాసం* create_Force_Read_సూచీ_వ్యక్తీకరణ(స్థిరరాశి VarForceInfo& varInfo,
                                                Ast_కణుపు_సమాసం* originalExprp,
                                                Ast_కణుపు_సమాసం* సూచీ_Exprp) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి flp = originalExprp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = originalExprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        // Must be an LValue to a static variable
        VN_AS(exprp->get_VAccess_లక్ష్యం_Recurse(), VarRef)->access(VAccess::READ);
        తిరిగి createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_సూచీ, exprp, originalExprp,
                                   సూచీ_Exprp);
    }

    స్థిర Ast_కణుపు_సమాసం* rebuild_Sel_మార్గం(Ast_కణుపు_సమాసం* pathp, Ast_కణుపు_సమాసం* పునాది_Exprp) {
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(pathp, Sel)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = rebuild_Sel_మార్గం(selp->fromp(), పునాది_Exprp);
            AstSel* స్థిరరాశి outp
                = కొత్త AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->వెడల్పు_Const()};
            outp->dtypeFrom(selp);
            తిరిగి outp;
        }
        తిరిగి పునాది_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
స్థిర శూన్య విభజించు_Deassign(AstDeassign* nodep) {
    AstConcat* స్థిరరాశి concatp = VN_CAST(nodep->lhsp(), Concat);
    ఒకవేళ (!concatp) తిరిగి;

    File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
    AstDeassign* స్థిరరాశి newLp = కొత్త AstDeassign{flp, concatp->lhsp()->unlink_Fr_వెనుక()};
    AstDeassign* స్థిరరాశి newRp = కొత్త AstDeassign{flp, concatp->rhsp()->unlink_Fr_వెనుక()};
    Ast_కణుపు_సమాసం* స్థిరరాశి conp = concatp->unlink_Fr_వెనుక();
    nodep->replaceWith(newLp);
    newLp->కూడు_తదుపరి_ఇక్కడ(newRp);
    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    VL_DO_DANGLING(conp->delete_వృక్షం(), conp);

    విభజించు_Deassign(newLp);
    విభజించు_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

తరగతి ForceDiscoveryVisitor final : బహిరంగ VNVisitorConst {
    ForceState& m_state;
    తార్కిక m_లోపల_Clocked_చురుకైన = అబద్ధం;

    శూన్య buildForceableUnpackedArray(AstVarScope* స్థిరరాశి nodep,
                                     AstUnpackArrayDType* స్థిరరాశి arrDtypep) {
        AstVar* స్థిరరాశి varp = nodep->varp();
        స్థిరరాశి std::సదిశ<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.ఖాళీలు(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి ఆకు_Dtypep = dims.వెనుక()->subDTypep()->skipRefp();
        స్థిరరాశి AstBasicDType* స్థిరరాశి innerBasicp = ఆకు_Dtypep->basicp();
        ఒకవేళ (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            తిరిగి;
        }

        File_పంక్తి* స్థిరరాశి flp = varp->fileline();
        స్థిరరాశి పూర్ణాంకం inner_వెడల్పు = ఆకు_Dtypep->వెడల్పు();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* స్థిరరాశి enVscp = info.m_forceEnVscp;
        AstVarScope* స్థిరరాశి valVscp = info.m_forceValVscp;

        Ast_Sen_అంశం* స్థిరరాశి itemsp = కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_మారింది,
                                                  కొత్త AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "force-update", కొత్త Ast_Sen_వృక్షం{flp, itemsp}};
        activep->sen_వృక్షం_Storep(activep->sentreep());

        Ast_కణుపు_Stmt* స్థిరరాశి alwaysBodyHeadp = ForceState::foreach_Unpacked_ఆకు(
            dims, [&](స్థిరరాశి std::సదిశ<పూర్ణాంకం>& idx, పూర్ణాంకం flat) -> Ast_కణుపు_Stmt* {
                AstVarRef* స్థిరరాశి origRefp = కొత్త AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_కణుపు_సమాసం* స్థిరరాశి origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_కణుపు_సమాసం* స్థిరరాశి enSelp = ForceState::buildNestedArraySel(
                    flp, కొత్త AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_కణుపు_సమాసం* స్థిరరాశి valSelp = ForceState::buildNestedArraySel(
                    flp, కొత్త AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_కణుపు_సమాసం* స్థిరరాశి forceExprp = కొత్త AstOr{
                    flp, కొత్త AstAnd{flp, enSelp, valSelp},
                    కొత్త AstAnd{flp, కొత్త AstNot{flp, enSelp->clone_వృక్షం_శుద్ధ(అబద్ధం)}, origSelp}};
                Ast_కణుపు_సమాసం* స్థిరరాశి lhsSelp = ForceState::buildNestedArraySel(
                    flp, కొత్త AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_కేటాయించు_Force* స్థిరరాశి forceAssignp = కొత్త Ast_కేటాయించు_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(నిజం);

                Ast_కణుపు_సమాసం* స్థిరరాశి rhsClonep = forceExprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                rhsClonep->foreach([varp](AstVarRef* స్థిరరాశి r) {
                    ఒకవేళ (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.కూడు_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_వెడల్పు - 1,
                                           /*hasArraySel=*/నిజం, /*skipRdRefresh=*/నిజం);
                తిరిగి forceAssignp;
            });
        activep->కూడు_Stmtsp(కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, శూన్యసూచిక, alwaysBodyHeadp});
        nodep->scopep()->కూడు_Blocksp(activep);
    }

    శూన్య సందర్శించు(Ast_కేటాయించు_Force* nodep) override {
        ఒకవేళ (nodep->user2()) తిరిగి;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* స్థిరరాశి lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* స్థిరరాశి forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, నిజం);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsExprp = nodep->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం);

        m_state.కూడు_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        ఒకవేళ (m_state.doing_కేటాయించు() && m_లోపల_Clocked_చురుకైన) {
            ఒకవేళ (AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        VL_RESTORER(m_లోపల_Clocked_చురుకైన);
        m_లోపల_Clocked_చురుకైన = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstVarScope* nodep) override {
        ఒకవేళ (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            ఒకవేళ (m_state.doing_కేటాయించు()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                తిరిగి;
            }

            ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                తిరిగి;
            }

            స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = nodep->varp()->basicp();
            ఒకవేళ (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            // Build the per-signal force update logic.
            AstVar* స్థిరరాశి varp = nodep->varp();
            File_పంక్తి* స్థిరరాశి flp = varp->fileline();
            ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
            AstVarScope* స్థిరరాశి enVscp = info.m_forceEnVscp;
            AstVarScope* స్థిరరాశి valVscp = info.m_forceValVscp;

            // Build an update block triggered by force-enable changes.
            Ast_Sen_అంశం* స్థిరరాశి itemsp = కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_మారింది,
                                                      కొత్త AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_చురుకైన* స్థిరరాశి activep
                = కొత్త Ast_చురుకైన{flp, "force-update", కొత్త Ast_Sen_వృక్షం{flp, itemsp}};
            activep->sen_వృక్షం_Storep(activep->sentreep());

            // Build expression selecting forced value when enabled, otherwise original value.
            // forceExpr = (isBitwise) ? ((en & val) | (~en & orig)) : (en ? val : orig);
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, nodep, VAccess::READ};
            ForceState::markNonReplaceable(refp);
            AstVarRef* స్థిరరాశి enRefp = కొత్త AstVarRef{flp, enVscp, VAccess::READ};
            AstVarRef* స్థిరరాశి valRefp = కొత్త AstVarRef{flp, valVscp, VAccess::READ};
            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = varp->dtypep()->skipRefp()->basicp();
            Ast_కణుపు_సమాసం* స్థిరరాశి forceExprp
                = basicp && basicp->isRanged()
                      ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstOr{
                            flp, కొత్త AstAnd{flp, enRefp, valRefp},
                            కొత్త AstAnd{flp, కొత్త AstNot{flp, enRefp->clone_వృక్షం_శుద్ధ(అబద్ధం)}, refp}})
                      : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstCond{flp, enRefp, valRefp, refp});
            Ast_కేటాయించు_Force* స్థిరరాశి forceAssignp
                = కొత్త Ast_కేటాయించు_Force{flp, కొత్త AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(నిజం);
            activep->కూడు_Stmtsp(కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, శూన్యసూచిక, forceAssignp});
            nodep->scopep()->కూడు_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsClonep = forceExprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            rhsClonep->foreach([varp](AstVarRef* స్థిరరాశి refp) {
                ఒకవేళ (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            స్థిరరాశి తార్కిక bitwiseVar = ForceState::isBitwiseDType(varp);
            స్థిరరాశి పూర్ణాంకం padMsb = bitwiseVar ? (varp->వెడల్పు() - 1) : 0;
            పూర్ణాంకం rangeLsb = 0;
            పూర్ణాంకం rangeMsb = padMsb;
            ఒకవేళ (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.కూడు_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, అబద్ధం);
        }
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్పష్టమైన ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_తదుపరి_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

తరగతి ForceConvertVisitor final : బహిరంగ VNVisitor {
    ForceState& m_state;

    శూన్య సందర్శించు(Ast_కేటాయించు_Force* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        AstVarRef* స్థిరరాశి lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* స్థిరరాశి forcedVarp = lhsVarRefp->varp();

        స్థిరరాశి ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        స్థిరరాశి ForceState::VarForceInfo* స్థిరరాశి varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        Ast_కేటాయించు* స్థిరరాశి rhsAssignp
            = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం)};

        Ast_కేటాయించు* valAssignp = శూన్యసూచిక;
        Ast_కేటాయించు* enAssignp = శూన్యసూచిక;
        స్థిరరాశి తార్కిక bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        ఒకవేళ (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_కణుపు_సమాసం* పునాది_Exprp = nodep->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
            పునాది_Exprp->foreach(
                [](AstVarRef* స్థిరరాశి refp) { ForceState::markNonReplaceable(refp); });
            ఒకవేళ (bitwiseForcedVar) {
                పునాది_Exprp = ForceState::zero_Pad_గమ్యం_పునాది_వెడల్పు(పునాది_Exprp, forcedVarp->వెడల్పు(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            ఒకవేళ (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* స్థిరరాశి ముసుగు_Constp = ForceState::make_Range_ముసుగు_Const(
                    nodep, forcedVarp->వెడల్పు(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_కణుపు_సమాసం* స్థిరరాశి valReadp
                    = కొత్త AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_కణుపు_సమాసం* స్థిరరాశి valWritep
                    = కొత్త AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_కణుపు_సమాసం* స్థిరరాశి notMaskp = కొత్త AstNot{flp, ముసుగు_Constp};
                Ast_కణుపు_సమాసం* స్థిరరాశి maskedOldp = కొత్త AstAnd{flp, valReadp, notMaskp};
                Ast_కణుపు_సమాసం* స్థిరరాశి newValp = కొత్త AstOr{flp, maskedOldp, పునాది_Exprp};
                valAssignp = కొత్త Ast_కేటాయించు{flp, valWritep, newValp};

                Ast_కణుపు_సమాసం* స్థిరరాశి enReadp
                    = కొత్త AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_కణుపు_సమాసం* స్థిరరాశి enWritep
                    = కొత్త AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_కణుపు_సమాసం* స్థిరరాశి newEnp
                    = కొత్త AstOr{flp, enReadp, ముసుగు_Constp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                enAssignp = కొత్త Ast_కేటాయించు{flp, enWritep, newEnp};
            } లేకపోతే {
                AstConst* స్థిరరాశి oneConstp = ForceState::make_Range_ముసుగు_Const(nodep, 1, 0, 0);
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsValp = ForceState::cast_గమ్యం_కణుపు_DType(పునాది_Exprp, forcedVarp);
                valAssignp = కొత్త Ast_కేటాయించు{
                    flp, కొత్త AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = కొత్త Ast_కేటాయించు{
                    flp, కొత్త AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        స్థిరరాశి AstSel* స్థిరరాశి selLhsp = VN_CAST(lhsp, Sel);
        స్థిరరాశి తార్కిక array_బిట్టు_Sel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_has_బిట్టు_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->వెడల్పు();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsDatap = ForceState::build_Rhs_దత్తాంశం_సమాసం(flp, info);
        AstCExpr* స్థిరరాశి rhsAddrp = కొత్త AstCExpr{flp};
        rhsAddrp->కూడు("&(");
        rhsAddrp->కూడు(rhsDatap);
        rhsAddrp->కూడు(")");
        AstCMethodHard* స్థిరరాశి కూడు_Force_Callp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_కూడు,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        కూడు_Force_Callp->కూడు_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        కూడు_Force_Callp->కూడు_Pinsp(rhsAddrp);
        కూడు_Force_Callp->కూడు_Pinsp(
            ForceState::makeConst32(flp, array_బిట్టు_Sel ? info.m_padLsb : info.m_rangeLsb));
        ఒకవేళ (array_బిట్టు_Sel) {
            కూడు_Force_Callp->కూడు_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            కూడు_Force_Callp->కూడు_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            కూడు_Force_Callp->కూడు_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->వెడల్పు()));
        }
        కూడు_Force_Callp->dtype_సమితి_Void();
        Ast_కణుపు_Stmt* స్థిరరాశి stmtp = కూడు_Force_Callp->makeStmt();

        Ast_కణుపు* tailp = rhsAssignp;
        ఒకవేళ (valAssignp) {
            tailp->కూడు_తదుపరి_ఇక్కడ(valAssignp);
            tailp = valAssignp;
        }
        ఒకవేళ (enAssignp) {
            tailp->కూడు_తదుపరి_ఇక్కడ(enAssignp);
            tailp = enAssignp;
        }
        tailp->కూడు_తదుపరి_ఇక్కడ(stmtp);
        ఒకవేళ (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->కూడు_తదుపరి_ఇక్కడ(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        AstVarRef* స్థిరరాశి lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* స్థిరరాశి releasedVarp = lhsVarRefp->varp();

        స్థిరరాశి ForceState::VarForceInfo* స్థిరరాశి varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        ఒకవేళ (!varInfo) {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        UASSERT_OBJ(!varInfo->m_forces.ఖాళీలు(), nodep, "Var info for variable with no forces");

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        స్థిరరాశి ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, అబద్ధం);

        స్థిరరాశి AstSel* స్థిరరాశి selLhsp = VN_CAST(lhsp, Sel);
        స్థిరరాశి తార్కిక array_బిట్టు_Sel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_has_బిట్టు_Sel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->వెడల్పు();
        AstCMethodHard* స్థిరరాశి releaseCallp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->కూడు_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        ఒకవేళ (array_బిట్టు_Sel) {
            releaseCallp->కూడు_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->కూడు_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_సమితి_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_కణుపు_Stmt* స్థిరరాశి releasep = releaseCallp->makeStmt();

        Ast_కేటాయించు* clearEnp = శూన్యసూచిక;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        ఒకవేళ (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_కణుపు_సమాసం* స్థిరరాశి enWritep
                = కొత్త AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            ఒకవేళ (ForceState::isBitwiseDType(releasedVarp)) {
                స్థిరరాశి పూర్ణాంకం var_వెడల్పు = releasedVarp->వెడల్పు();
                ఒకవేళ (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_వెడల్పు - 1) {
                    clearEnp
                        = కొత్త Ast_కేటాయించు{flp, enWritep, ForceState::makeZeroConst(nodep, var_వెడల్పు)};
                } లేకపోతే {
                    // forceEn = forceEn & ~mask(range);
                    Ast_కణుపు_సమాసం* స్థిరరాశి enReadp
                        = కొత్త AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* స్థిరరాశి ముసుగు_Const = ForceState::make_Range_ముసుగు_Const(
                        nodep, var_వెడల్పు, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_కణుపు_సమాసం* స్థిరరాశి newEnp
                        = కొత్త AstAnd{flp, enReadp, కొత్త AstNot{flp, ముసుగు_Const}};
                    clearEnp = కొత్త Ast_కేటాయించు{flp, enWritep, newEnp};
                }
            } లేకపోతే {
                clearEnp = కొత్త Ast_కేటాయించు{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(lhsp, Sel);
        Ast_కణుపు_సమాసం* స్థిరరాశి basep = selp ? selp->fromp() : lhsp;

        Ast_కణుపు* stmtListp = releasep;
        ఒకవేళ (clearEnp) {
            clearEnp->కూడు_తదుపరి_ఇక్కడ(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        స్థిరరాశి తార్కిక నిండిన_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->వెడల్పు() - 1;
        ఒకవేళ (!releasedVarp->isContinuously()
            && !(m_state.doing_కేటాయించు() && m_state.hasClockedWrite(releasedVarp)
                 && నిండిన_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            స్థిరరాశి తార్కిక has_Opaque_మార్గం = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_కణుపు_సమాసం* forceReadp = శూన్యసూచిక;
            ఒకవేళ (has_Opaque_మార్గం) {
                forceReadp = m_state.create_Force_Read_సూచీ_వ్యక్తీకరణ(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } లేకపోతే ఒకవేళ (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_సూచీ_వ్యక్తీకరణ(
                    *varInfo, basep,
                    ForceState::build_Flatten_సూచీ_సమాసం(flp, rangeInfo.m_arrayInfo));
                ఒకవేళ (selp) { forceReadp = ForceState::rebuild_Sel_మార్గం(lhsp, forceReadp); }
            } లేకపోతే {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_మార్గం(
                                 lhsp, m_state.create_Force_Read_వ్యక్తీకరణ(*varInfo, lhsVarRefp))
                           : m_state.create_Force_Read_వ్యక్తీకరణ(*varInfo, lhsVarRefp);
            }
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{flp, lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం), forceReadp};
            assignp->కూడు_తదుపరి_ఇక్కడ(stmtListp);
            stmtListp = assignp;
        }

        ఒకవేళ (varInfo->m_forceRdVscp) stmtListp->కూడు_తదుపరి(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_తదుపరి_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

తరగతి ForceReplaceVisitor final : బహిరంగ VNVisitor {
    స్థిరరాశి ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_కణుపు_Stmt* m_stmtp = శూన్యసూచిక;
    తార్కిక m_లోపల_Logic = అబద్ధం;

    శూన్య iterateLogic(Ast_కణుపు* nodep) {
        VL_RESTORER(m_లోపల_Logic);
        m_లోపల_Logic = నిజం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        ఒకవేళ (AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp()->పునాది_Fromp(నిజం), VarRef)) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->కూడు_తదుపరి_ఇక్కడ(updatep);
            }
        }
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_తదుపరి_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    శూన్య సందర్శించు(AstDeassign*) override {}
    శూన్య సందర్శించు(AstCFunc* nodep) override { iterateLogic(nodep); }
    శూన్య సందర్శించు(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override { iterateLogic(nodep); }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::CONT_కేటాయించు) {
            iterateChildren(nodep);
            తిరిగి;
        }
        iterateLogic(nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override { iterateLogic(nodep); }
    శూన్య సందర్శించు(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->fromp(), VarRef);
        ఒకవేళ (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
            తిరిగి;
        }

        AstVar* స్థిరరాశి varp = refp->varp();
        స్థిరరాశి ForceState::VarForceInfo* స్థిరరాశి varInfo = m_state.getVarInfo(refp->varScopep());
        ఒకవేళ (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.ఖాళీలు()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
            తిరిగి;
        }

        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            స్థిరరాశి పూర్ణాంకం selLsb = lsbConstp->గమ్యం_SInt();
            స్థిరరాశి పూర్ణాంకం selMsb = selLsb + nodep->వెడల్పు() - 1;
            ఒకవేళ (!varp->is_Sig_ప్రజా()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
                తిరిగి;
            }
        }

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* స్థిరరాశి refClonep = refp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->వెడల్పు())};
        callp->కూడు_Pinsp(refClonep);
        callp->కూడు_Pinsp(nodep->lsbp()->clone_వృక్షం_శుద్ధ(అబద్ధం));
        callp->కూడు_Pinsp(ForceState::makeConst32(flp, nodep->వెడల్పు()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి వెనుక_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            ఒకవేళ (వెనుక_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                తిరిగి;
            }
        }

        Ast_కణుపు* స్థిరరాశి basep = nodep->పునాది_Fromp(నిజం);
        AstVarRef* స్థిరరాశి పునాది_Refp = VN_CAST(basep, VarRef);
        ఒకవేళ (!పునాది_Refp) {
            iterateChildren(nodep);
            తిరిగి;
        }
        AstVar* స్థిరరాశి varp = పునాది_Refp->varp();
        స్థిరరాశి ForceState::VarForceInfo* స్థిరరాశి varInfo = m_state.getVarInfo(పునాది_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        ఒకవేళ (m_state.isNotReplaceable(పునాది_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            తిరిగి;
        }

        ఒకవేళ (!పునాది_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            తిరిగి;
        }
        ఒకవేళ (m_state.doing_కేటాయించు() && varInfo->m_forceRdVscp) {
            పునాది_Refp->varp(varInfo->m_forceRdVscp->varp());
            పునాది_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            తిరిగి;
        }
        స్థిరరాశి ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        కొరకు (AstArraySel* స్థిరరాశి selp : arrayInfo.m_sels) iterate_And_తదుపరి_Null(selp->bitp());
        Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_Exprp
            = ForceState::build_Flatten_సూచీ_సమాసం(nodep->fileline(), arrayInfo);
        Ast_కణుపు_సమాసం* స్థిరరాశి readExprp
            = m_state.create_Force_Read_సూచీ_వ్యక్తీకరణ(*varInfo, nodep, సూచీ_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (m_state.isNotReplaceable(nodep)) తిరిగి;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి వెనుక_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            ఒకవేళ (వెనుక_Selp->fromp() == nodep) తిరిగి;
        }

        స్థిరరాశి ForceState::VarForceInfo* స్థిరరాశి varInfo = m_state.getVarInfo(nodep->varScopep());
        ఒకవేళ (!varInfo) తిరిగి;

        ఒకవేళ (varInfo->m_forceRdVscp) {
            ఒకవేళ (nodep->access().isRW()) {
                ఒకవేళ (m_లోపల_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                తిరిగి;
            }
            ఒకవేళ (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_కణుపు_సమాసం* స్థిరరాశి readp
                    = m_state.create_Force_Rd_వ్యక్తీకరణ(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
            ఒకవేళ (m_లోపల_Logic && m_stmtp) {
                m_stmtp->కూడు_తదుపరి_ఇక్కడ(m_state.createForceRdUpdateStmt(*varInfo));
            }
            తిరిగి;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        ఒకవేళ (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            తిరిగి;
        }

        UASSERT_OBJ(!varInfo->m_forces.ఖాళీలు(), nodep, "Var info for variable with no forces");

        ఒకవేళ (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } లేకపోతే ఒకవేళ (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి readExprp = m_state.create_Force_Read_వ్యక్తీకరణ(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (ForceState::is_Outermost_Opaque_మార్గం_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_AS(nodep, కణుపు_సమాసం);
            Ast_కణుపు* స్థిరరాశి basep = exprp->పునాది_Fromp(నిజం);
            AstVarRef* స్థిరరాశి పునాది_Refp = VN_CAST(basep, VarRef);
            ఒకవేళ (పునాది_Refp) {
                AstVar* స్థిరరాశి varp = పునాది_Refp->varp();
                ఒకవేళ (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    స్థిరరాశి ForceState::VarForceInfo* స్థిరరాశి varInfo
                        = m_state.getVarInfo(పునాది_Refp->varScopep());
                    ఒకవేళ (!m_state.isNotReplaceable(పునాది_Refp) && varInfo) {
                        స్థిరరాశి పూర్ణాంకం force_మార్గం_సూచీ = varInfo->కనుగొను_Force_మార్గం_సూచీ(exprp);
                        ఒకవేళ (force_మార్గం_సూచీ >= 0) {
                            ఒకవేళ (!పునాది_Refp->access().isReadOnly()) తిరిగి;
                            Ast_కణుపు_సమాసం* స్థిరరాశి readExprp = m_state.create_Force_Read_సూచీ_వ్యక్తీకరణ(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_మార్గం_సూచీ));
                            nodep->replaceWith(readExprp);
                            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                            తిరిగి;
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }

బహిరంగ:
    స్పష్టమైన ForceReplaceVisitor(AstNetlist* nodep, స్థిరరాశి ForceState& state)
        : m_state{state} {
        iterate_And_తదుపరి_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::కూడు_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

స్థిర శూన్య forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_సమితి& permanentlyProtected,
                         ForceState::Built_Var_Scope_సమితి& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ఒకవేళ (!v3Global.has_Forceable_సంకేతాలు()) తిరిగి;
    ForceState state{అబద్ధం, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_వృక్షం("force", 0, dump_వృక్షం_Either_Level() >= 3);
}

స్థిర శూన్య కేటాయించు_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_సమితి& permanentlyProtected,
                          ForceState::Built_Var_Scope_సమితి& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    ఒకవేళ (!v3Global.has_కేటాయించు_Deassign()) తిరిగి;

    std::సదిశ<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.నెట్టు_వెనుక(deassignp); });
    కొరకు (AstDeassign* స్థిరరాశి deassignp : deassignps) విభజించు_Deassign(deassignp);

    std::సదిశ<Ast_కేటాయించు_Cont*> కేటాయించు_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_కణుపు_Stmt* nodep) {
        ఒకవేళ (Ast_కేటాయించు_Cont* స్థిరరాశి కేటాయించు_Contp = VN_CAST(nodep, కేటాయించు_Cont)) {
            కేటాయించు_Contps.నెట్టు_వెనుక(కేటాయించు_Contp);
        } లేకపోతే ఒకవేళ (AstDeassign* స్థిరరాశి deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.నెట్టు_వెనుక(deassignp);
        }
    });

    కొరకు (Ast_కేటాయించు_Cont* స్థిరరాశి assignp : కేటాయించు_Contps) {
        assignp->replaceWith(కొత్త Ast_కేటాయించు_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_వెనుక(),
                                                assignp->rhsp()->unlink_Fr_వెనుక()});
        VL_DO_DANGLING(assignp->delete_వృక్షం(), assignp);
    }
    కొరకు (AstDeassign* స్థిరరాశి deassignp : deassignps) {
        deassignp->replaceWith(
            కొత్త AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_వృక్షం_శుద్ధ(నిజం)});
        VL_DO_DANGLING(deassignp->delete_వృక్షం(), deassignp);
    }
    ForceState state{నిజం, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_వృక్షం("assign-deassign", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3Force::force_And_కేటాయించు_All(AstNetlist* nodep) {
    స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_సమితి permanentlyProtected;
    ForceState::Built_Var_Scope_సమితి forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    కేటాయించు_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
