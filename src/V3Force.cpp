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
        AstVarScope* m_rhsVarVscp = शून्यसूचकः;  // Scope of the var containing RHSID
        Ast_ग्रन्थिः_व्यञ्जकम्* m_rhsExprp = शून्यसूचकः;  // Expression on RHS of this force assignment

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
        AstVarScope* m_forceVecVscp = शून्यसूचकः;  // Scope of the __VforceVec (VlForceVec) variable
        AstVarScope* m_forceRdVscp = शून्यसूचकः;  // Scope of the __VforceRd (read-back) variable
        AstVarScope* m_forceEnVscp = शून्यसूचकः;  // Scope of the __VforceEn (force enable) variable
        AstVarScope* m_forceValVscp = शून्यसूचकः;  // Scope of the __VforceVal (force value) variable
        AstVarScope* m_varVscp = शून्यसूचकः;  // Scope of the forced (original) variable
        AstVar* m_varp = शून्यसूचकः;  // The forced (original) variable
        AstScope* m_scopep = शून्यसूचकः;  // Scope containing the forced variable
        std::unordered_map<Ast_नियोजय_Force*, ForceInfo> m_forces;
        std::unordered_map<string, पूर्णाङ्क> m_force_मार्गः_गन्तव्यम्_सूचकांक;
        पूर्णाङ्क m_अग्र_Force_मार्गः_सूचकांक = 1;  // Start at 1 so 0 can be the base path (whole signal)

    गुप्त:
        स्थिर शून्य build_Force_मार्गः_कुञ्चिका_Recurse(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, string& बहिर्गमनम्) {
            यदि (VN_IS(nodep, VarRef)) प्रत्यागम;
            यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
                build_Force_मार्गः_कुञ्चिका_Recurse(selp->fromp(), बहिर्गमनम्);
                बहिर्गमनम् += "|S" + cvt_गन्तव्यम्_Str(selp->lsbConst()) + ":" + cvt_गन्तव्यम्_Str(selp->विस्तारः_Const());
                प्रत्यागम;
            }
            यदि (AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = selp->fromp();
                build_Force_मार्गः_कुञ्चिका_Recurse(fromp, बहिर्गमनम्);
                बहिर्गमनम् += "|T" + selp->name();
                प्रत्यागम;
            }
            nodep->v3fatalSrc("Unsupported: opaque force path selector "
                              << nodep->prettyTypeName());
        }

        स्थिर string force_मार्गः_कुञ्चिका(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
            string बहिर्गमनम्;
            build_Force_मार्गः_कुञ्चिका_Recurse(nodep, बहिर्गमनम्);
            प्रत्यागम बहिर्गमनम्;
        }

    सार्वजनिक:
        पूर्णाङ्क get_Or_Create_Force_मार्गः_सूचकांक(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
            ध्रुव स्वचालित pair
                = m_force_मार्गः_गन्तव्यम्_सूचकांक.emplace(force_मार्गः_कुञ्चिका(nodep), m_अग्र_Force_मार्गः_सूचकांक);
            यदि (pair.second) ++m_अग्र_Force_मार्गः_सूचकांक;
            प्रत्यागम pair.first->second;
        }

        पूर्णाङ्क find_Force_मार्गः_सूचकांक(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) ध्रुव {
            ध्रुव स्वचालित it = m_force_मार्गः_गन्तव्यम्_सूचकांक.find(force_मार्गः_कुञ्चिका(nodep));
            प्रत्यागम it != m_force_मार्गः_गन्तव्यम्_सूचकांक.अन्तः_2() ? it->second : -1;
        }
    };

    संरचना ForceHelperVars final {
        AstVar* m_rdVarp = शून्यसूचकः;  // The __VforceRd (read-back) variable
        AstVar* m_enVarp = शून्यसूचकः;  // The __VforceEn (force enable) variable
        AstVar* m_valVarp = शून्यसूचकः;  // The __VforceVal (force value) variable
    };

    संरचना ArraySelInfo final {
        std::सदिशम्<AstArraySel*> m_sels;  // Array selects collected along the path, outer first
        तार्किक m_hasBitSel = असत्य;  // Bit-select was found along the path too
    };

    संरचना ForceRangeInfo final : ForceRange {
        तार्किक m_hasArraySel = असत्य;  // LHS has an array select
        ArraySelInfo m_arrayInfo;  // Array-select information collected for the LHS
    };

गुप्त:
    उपयुज्य ScopeVarCache = std::unordered_map<ध्रुव AstVar*, AstVarScope*>;

    // NODE STATE
    //  AstVarRef::user1      -> bool.  Not to replace reference
    //  AstAssignForce::user2 -> bool.  Force is synthetic (externally forceable)
    //  AstVar::user3         -> ForceHelperVars via m_forceHelperVarsByVar
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;
    ध्रुव VNUser2_अन्तर्_Use m_user2_अन्तर्_Use;

सार्वजनिक:
    उपयुज्य ForceHelperVarsByVar = AstUser3Allocator<AstVar, ForceHelperVars>;
    // Raw-variable reads never to replace, even in the other pass (see markPermanentlyProtected)
    उपयुज्य Protected_Var_Ref_समुच्चयः = std::unordered_समुच्चयः<ध्रुव AstVarRef*>;
    // AstVarScopes with force-rd-update logic already generated, by either pass
    उपयुज्य Built_Var_Scope_समुच्चयः = std::unordered_समुच्चयः<ध्रुव AstVarScope*>;

गुप्त:
    ForceHelperVarsByVar& m_forceHelperVarsByVar;
    std::सदिशम्<VarForceInfo> m_varInfos;  // Indexed by stable variable ID
    std::unordered_map<AstVarScope*, पूर्णाङ्क> m_var_गन्तव्यम्_Id;
    std::unordered_समुच्चयः<AstVar*> m_clockedWrites;
    std::unordered_map<AstVar*, std::सदिशम्<ForceInfo*>> m_rhs_Dep_गन्तव्यम्_Forces;
    std::unordered_map<AstScope*, ScopeVarCache> m_scopeVarCaches;
    तार्किक m_doing_नियोजय = असत्य;  // If true, we're processing procedural continuous assign
                                 // statements instead of force statements
    Protected_Var_Ref_समुच्चयः& m_permanentlyProtected;
    Built_Var_Scope_समुच्चयः& m_forceRdUpdateBuilt;

सार्वजनिक:
    ForceState(तार्किक doing_नियोजय, ForceHelperVarsByVar& forceHelperVarsByVar,
               Protected_Var_Ref_समुच्चयः& permanentlyProtected, Built_Var_Scope_समुच्चयः& forceRdUpdateBuilt)
        : m_forceHelperVarsByVar{forceHelperVarsByVar}
        , m_doing_नियोजय{doing_नियोजय}
        , m_permanentlyProtected{permanentlyProtected}
        , m_forceRdUpdateBuilt{forceRdUpdateBuilt} {}
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    स्थिर AstConst* makeZeroConst(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः) {
        V3_सङ्ख्या zero{nodep, विस्तारः};
        zero.समुच्चयः_All_Bits0();
        प्रत्यागम नूतन AstConst{nodep->fileline(), zero};
    }

    स्थिर AstConst* makeConst32(File_पङ्क्तिः* flp, पूर्णाङ्क मानम्) {
        प्रत्यागम नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, static_cast<uint32_t>(मानम्)};
    }

    स्थिर AstConst* make_Range_आवरणम्_Const(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        V3_सङ्ख्या आवरणम्{nodep, विस्तारः};
        आवरणम्.समुच्चयः_All_Bits0();
        प्रति (पूर्णाङ्क bit = lsb; bit <= msb; ++bit) आवरणम्.समुच्चयः_Bit(bit, 1);
        प्रत्यागम नूतन AstConst{nodep->fileline(), आवरणम्};
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* zero_Pad_गन्तव्यम्_आधारः_विस्तारः(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क आधारः_विस्तारः, पूर्णाङ्क padLsb,
                                           पूर्णाङ्क padMsb) {
        यदि (आधारः_विस्तारः <= 0) प्रत्यागम exprp;
        ध्रुव पूर्णाङ्क नीचः_Pad = padLsb;
        ध्रुव पूर्णाङ्क उच्चः_Pad = आधारः_विस्तारः - (padMsb + 1);
        यदि (नीचः_Pad > 0) {
            exprp = नूतन AstConcat{exprp->fileline(), exprp, makeZeroConst(exprp, नीचः_Pad)};
        }
        यदि (उच्चः_Pad > 0) {
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

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* cast_गन्तव्यम्_ग्रन्थिः_DType(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः* dtypeFromp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = dtypeFromp->dtypep()->skipRefp();
        ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp();
        यदि (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            प्रत्यागम exprp;
        }
        प्रत्यागम नूतन AstCCast{exprp->fileline(), exprp, dtypeFromp};
    }

    स्थिर शून्य markNonReplaceable(AstVarRef* ध्रुव nodep) { nodep->user1_समुच्चयः_Once(); }
    तार्किक isNotReplaceable(ध्रुव AstVarRef* ध्रुव nodep) ध्रुव {
        प्रत्यागम nodep->user1() || m_permanentlyProtected.गणनम्(nodep) > 0;
    }
    // Like markNonReplaceable(), but persists into the other of the force/assign pass too
    शून्य markPermanentlyProtected(AstVarRef* ध्रुव nodep) ध्रुव {
        markNonReplaceable(nodep);
        m_permanentlyProtected.निवेशय(nodep);
    }

    स्थिर std::सदिशम्<ForceInfo*> force_Infos_अन्तर्_Id_क्रमः(VarForceInfo& info) {
        std::सदिशम्<ForceInfo*> forceps;
        forceps.reserve(info.m_forces.आकार());
        प्रति (स्वचालित& it : info.m_forces) forceps.प्रक्षिप_पृष्ठभागः(&it.second);
        std::क्रमय(forceps.begin(), forceps.अन्तः_2(), [](ध्रुव ForceInfo* ap, ध्रुव ForceInfo* bp) {
            प्रत्यागम ap->m_forceId < bp->m_forceId;
        });
        प्रत्यागम forceps;
    }

    स्थिर std::सदिशम्<ध्रुव ForceInfo*> force_Infos_अन्तर्_Id_क्रमः(ध्रुव VarForceInfo& info) {
        std::सदिशम्<ध्रुव ForceInfo*> forceps;
        forceps.reserve(info.m_forces.आकार());
        प्रति (ध्रुव स्वचालित& it : info.m_forces) forceps.प्रक्षिप_पृष्ठभागः(&it.second);
        std::क्रमय(forceps.begin(), forceps.अन्तः_2(), [](ध्रुव ForceInfo* ap, ध्रुव ForceInfo* bp) {
            प्रत्यागम ap->m_forceId < bp->m_forceId;
        });
        प्रत्यागम forceps;
    }

    स्थिर तार्किक is_Opaque_मार्गः_Selector(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम VN_IS(nodep, Sel) || VN_IS(nodep, ग्रन्थिः_Sel) || VN_IS(nodep, StructSel);
    }

    स्थिर तार्किक is_Outermost_Opaque_मार्गः_Selector(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (!is_Opaque_मार्गः_Selector(nodep)) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        प्रत्यागम !backp || !is_Opaque_मार्गः_Selector(backp);
    }

    स्थिर AstVarRef* getOneVarRef(Ast_ग्रन्थिः_व्यञ्जकम्* forceStmtp) {
        Ast_ग्रन्थिः* ध्रुव basep = forceStmtp->आधारः_Fromp(सत्य);
        यदि (AstSampled* sampledp = VN_CAST(basep, Sampled))
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* exprp = VN_CAST(sampledp->exprp(), ग्रन्थिः_व्यञ्जकम्))
                प्रत्यागम getOneVarRef(exprp);
        AstVarRef* ध्रुव varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "Force/release expression has no VarRef at its base");
        प्रत्यागम varRefp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* buildNestedArraySel(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp,
                                            ध्रुव std::सदिशम्<पूर्णाङ्क>& indicies) {
        Ast_ग्रन्थिः_व्यञ्जकम्* curp = fromp;
        प्रति (ध्रुव पूर्णाङ्क idx : indicies) curp = नूतन AstArraySel{flp, curp, idx};
        प्रत्यागम curp;
    }

    प्रतिरूपम् <प्रकारनाम Fn>
    स्थिर Ast_ग्रन्थिः_Stmt* foreach_Unpacked_पर्णम्(ध्रुव std::सदिशम्<AstUnpackArrayDType*>& dims,
                                            Fn build_पर्णम्) {
        Ast_ग्रन्थिः_Stmt* headp = शून्यसूचकः;
        Ast_ग्रन्थिः_Stmt* tailp = शून्यसूचकः;
        यदि (dims.रिक्तानि()) प्रत्यागम शून्यसूचकः;
        पूर्णाङ्क योग = 1;
        प्रति (ध्रुव AstUnpackArrayDType* ध्रुव d : dims) योग *= d->elementsConst();
        यदि (योग <= 0) प्रत्यागम शून्यसूचकः;
        std::सदिशम्<पूर्णाङ्क> idx(dims.आकार(), 0);
        प्रति (पूर्णाङ्क flat = 0; flat < योग; ++flat) {
            Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = build_पर्णम्(idx, flat);
            यदि (!headp) {
                headp = stmtp;
            } अन्यथा {
                tailp->सङ्कलय_अग्र(stmtp);
            }
            tailp = stmtp;
            प्रति (पूर्णाङ्क d = static_cast<पूर्णाङ्क>(dims.आकार()) - 1; d >= 0; --d) {
                यदि (++idx[d] < dims[d]->elementsConst()) भङ्ग;
                idx[d] = 0;
            }
        }
        प्रत्यागम headp;
    }

    ForceRangeInfo getForceRangeInfo(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, AstVar* varp,
                                     तार्किक requireConstRangeSelect) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.रिक्तानि();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->विस्तारः() - 1) : 0;

        यदि (ध्रुव AstSel* ध्रुव outerSelp = VN_CAST(lhsp, Sel)) {
            पूर्णाङ्क योग_Lsb = 0;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* curp = lhsp; ध्रुव AstSel* ध्रुव selp = VN_CAST(curp, Sel);
                 curp = selp->fromp()) {
                यदि (requireConstRangeSelect) {
                    UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                                "Unsupported: force on non-const range select");
                }
                योग_Lsb += selp->lsbConst();
            }
            info.m_padLsb = योग_Lsb;
            info.m_padMsb = योग_Lsb + outerSelp->विस्तारः_Const() - 1;
        }

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        यदि (info.m_hasArraySel) {
            // Unpacked array selections are tracked as a single flattened element index
            // inside VlForceVec, regardless of how many unpacked dimensions the source uses.
            std::सदिशम्<पूर्णाङ्क> indices;
            indices.reserve(info.m_arrayInfo.m_sels.आकार());
            प्रति (AstArraySel* ध्रुव selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.प्रक्षिप_पृष्ठभागः(VN_AS(selp->bitp(), Const)->गन्तव्यम्_SInt());
            }
            info.m_rangeLsb = flatten_सूचकांक(indices, arraySelDimSizes(info.m_arrayInfo));

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
            ध्रुव पूर्णाङ्क सूचकांक = varInfo.get_Or_Create_Force_मार्गः_सूचकांक(lhsp);
            info.m_rangeLsb = सूचकांक;
            info.m_rangeMsb = सूचकांक;
        }
        प्रत्यागम info;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* सङ्कलय_Rhs_मानम्_Reads(ध्रुव VarForceInfo& varInfo, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) ध्रुव {
        यदि (!doing_नियोजय()) प्रत्यागम exprp;

        ध्रुव std::सदिशम्<ध्रुव ForceInfo*> forceps = force_Infos_अन्तर्_Id_क्रमः(varInfo);
        यदि (forceps.रिक्तानि()) प्रत्यागम exprp;

        // VlForceVec stores pointers to RHS shadows, so expose those reads to scheduling.
        AstCExpr* ध्रुव cexprp = नूतन AstCExpr{exprp->fileline(), AstCExpr::विशुद्धम्{}};
        cexprp->dtypeFrom(exprp);
        cexprp->सङ्कलय("(");
        प्रति (ध्रुव ForceInfo* ध्रुव finfop : forceps) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, exprp, "No RHS var for forced variable");
            AstVarRef* ध्रुव refp
                = नूतन AstVarRef{exprp->fileline(), finfop->m_rhsVarVscp, VAccess::READ};
            markNonReplaceable(refp);
            cexprp->सङ्कलय("(void)(");
            cexprp->सङ्कलय(refp);
            cexprp->सङ्कलय("), ");
        }
        cexprp->सङ्कलय(exprp);
        cexprp->सङ्कलय(")");
        प्रत्यागम cexprp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceReadCall(ध्रुव VarForceInfo& varInfo, File_पङ्क्तिः* flp, VCMethod विधानम्,
                                     Ast_ग्रन्थिः_व्यञ्जकम्* originalExprp, Ast_ग्रन्थिः* dtypeFromp,
                                     Ast_ग्रन्थिः_व्यञ्जकम्* सूचकांक_Exprp) ध्रुव {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        // Protect only the read this call replaces, which would otherwise be replaced
        // again and recurse.  Everything else in the expression is an ordinary read and
        // must still see its own force, including an index read of the same array as in
        // 'mem[mem[0]]'.
        markNonReplaceable(getOneVarRef(originalExprp));
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origValp
            = सङ्कलय_Rhs_मानम्_Reads(varInfo, cast_गन्तव्यम्_ग्रन्थिः_DType(originalExprp, dtypeFromp));

        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ}, विधानम्, origValp};
        यदि (सूचकांक_Exprp) callp->सङ्कलय_Pinsp(सूचकांक_Exprp);
        callp->dtypeFrom(dtypeFromp);
        प्रत्यागम callp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceRdExpression(ध्रुव VarForceInfo& varInfo, File_पङ्क्तिः* flp) ध्रुव {
        UASSERT(varInfo.m_varVscp, "No base var scope for forced variable");
        AstVar* ध्रुव varp = varInfo.m_varVscp->varp();
        यदि (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            // Array element forces live in the force vector, including partial-element forces.
            // The assign pass reuses the force-read shadow, as for an array element read.
            यदि (doing_नियोजय()) { प्रत्यागम नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::READ}; }
            // Whole-array force reads introduce artificial scheduling cycles.
            varp->fileline()->modifyWarnOff(V3_Error_संहिता::UNOPTFLAT, सत्य);
            AstVarRef* ध्रुव आधारः_Refp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
            markPermanentlyProtected(आधारः_Refp);
            प्रत्यागम createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, आधारः_Refp, varp,
                                       शून्यसूचकः);
        }
        AstVarRef* ध्रुव आधारः_Refp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
        markPermanentlyProtected(आधारः_Refp);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enRefp = नूतन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valRefp = नूतन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ};
        यदि (isBitwiseDType(varp)) {
            प्रत्यागम नूतन AstOr{
                flp, नूतन AstAnd{flp, enRefp, valRefp},
                नूतन AstAnd{flp, नूतन AstNot{flp, enRefp->clone_वृक्षः_विशुद्धम्(असत्य)}, आधारः_Refp}};
        }
        प्रत्यागम नूतन AstCond{flp, enRefp, valRefp, आधारः_Refp};
    }

    Ast_ग्रन्थिः_Stmt* createForceRdUpdateStmt(ध्रुव VarForceInfo& varInfo) ध्रुव {
        UASSERT(varInfo.m_forceRdVscp, "No forceRd for forced variable");
        AstVar* ध्रुव varp = varInfo.m_varVscp->varp();
        यदि (VN_IS(varp->dtypeSkipRefp(), UnpackArrayDType)) {
            प्रत्यागम createForceRdUpdateStmtUnpacked(varInfo);
        }
        File_पङ्क्तिः* ध्रुव flp = varInfo.m_varVscp->fileline();
        प्रत्यागम नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE},
                             createForceRdExpression(varInfo, flp)};
    }

    Ast_ग्रन्थिः_Stmt* createForceRdUpdateStmtUnpacked(ध्रुव VarForceInfo& varInfo) ध्रुव {
        File_पङ्क्तिः* ध्रुव flp = varInfo.m_varVscp->fileline();
        AstVar* ध्रुव varp = varInfo.m_varVscp->varp();
        AstUnpackArrayDType* ध्रुव arrDtypep = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType);
        ध्रुव std::सदिशम्<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtsp = foreach_Unpacked_पर्णम्(
            dims, [&](ध्रुव std::सदिशम्<पूर्णाङ्क>& idx, पूर्णाङ्क /*flat*/) -> Ast_ग्रन्थिः_Stmt* {
                AstVarRef* ध्रुव आधारः_Refp = नूतन AstVarRef{flp, varInfo.m_varVscp, VAccess::READ};
                markPermanentlyProtected(आधारः_Refp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव आधारः_Selp = buildNestedArraySel(flp, आधारः_Refp, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceEnVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceValVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp = नूतन AstOr{
                    flp, नूतन AstAnd{flp, enSelp, valSelp},
                    नूतन AstAnd{flp, नूतन AstNot{flp, enSelp->clone_वृक्षः_विशुद्धम्(असत्य)}, आधारः_Selp}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rdLhsSelp = buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, varInfo.m_forceRdVscp, VAccess::WRITE}, idx);
                प्रत्यागम नूतन Ast_नियोजय{flp, rdLhsSelp, readExprp};
            });
        // Whole-array reads use the force vector, whose external-force entries point to
        // RHS shadows. Refresh those alongside forceRd so blocking writes are visible
        // before the next statement. Only synthetic expressions are safe to reevaluate here.
        प्रति (ध्रुव ForceInfo* ध्रुव finfop : force_Infos_अन्तर्_Id_क्रमः(varInfo)) {
            यदि (!finfop->m_isExternal) अनुवर्त;
            stmtsp->सङ्कलय_अग्र(नूतन Ast_नियोजय{flp,
                                          नूतन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                          finfop->m_rhsExprp->clone_वृक्षः_विशुद्धम्(असत्य)});
        }
        प्रत्यागम stmtsp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVarScope* vscp) {
        ध्रुव स्वचालित it = m_var_गन्तव्यम्_Id.find(vscp);
        यदि (it != m_var_गन्तव्यम्_Id.अन्तः_2()) प्रत्यागम m_varInfos[it->second];

        m_var_गन्तव्यम्_Id.emplace(vscp, m_varInfos.आकार());
        m_varInfos.emplace_पृष्ठभागः();
        VarForceInfo& info = m_varInfos.पृष्ठभागः();
        info.m_varVscp = vscp;
        info.m_varp = vscp->varp();
        info.m_scopep = vscp->scopep();

        AstVar* ध्रुव varp = info.m_varp;
        यदि (!varp->isForceable()) प्रत्यागम info;

        File_पङ्क्तिः* ध्रुव flp = varp->fileline();
        ForceHelperVars& helperVars = m_forceHelperVarsByVar(varp);
        ध्रुव तार्किक helperVarsBuilt = helperVars.m_rdVarp != शून्यसूचकः;
        UASSERT_OBJ(helperVarsBuilt == (helperVars.m_enVarp != शून्यसूचकः)
                        && helperVarsBuilt == (helperVars.m_valVarp != शून्यसूचकः),
                    varp, "Incomplete force helper set");
        यदि (!helperVarsBuilt) {
            ध्रुव तार्किक unpacked = isUnpackedArrayDType(varp->dtypep());
            ध्रुव VVarType enValType = unpacked ? VVarType::WIRE : VVarType::VAR;
            Ast_ग्रन्थिः_DType* ध्रुव enDtypep
                = unpacked || isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
            helperVars.m_rdVarp
                = नूतन AstVar{flp, VVarType::WIRE, varp->name() + "__VforceRd", varp->dtypep()};
            helperVars.m_rdVarp->sig_लौकिकम्(सत्य);
            helperVars.m_enVarp
                = नूतन AstVar{flp, enValType, varp->name() + "__VforceEn", enDtypep};
            helperVars.m_enVarp->sigUserRWPublic(सत्य);
            helperVars.m_valVarp
                = नूतन AstVar{flp, enValType, varp->name() + "__VforceVal", varp->dtypep()};
            helperVars.m_valVarp->sigUserRWPublic(सत्य);
            varp->सङ्कलय_अग्र_अत्र(helperVars.m_rdVarp);
            varp->सङ्कलय_अग्र_अत्र(helperVars.m_enVarp);
            varp->सङ्कलय_अग्र_अत्र(helperVars.m_valVarp);
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
            info.m_scopep->सङ्कलय_Varsp(info.m_forceRdVscp);
            info.m_scopep->सङ्कलय_Varsp(info.m_forceEnVscp);
            info.m_scopep->सङ्कलय_Varsp(info.m_forceValVscp);
        }
        प्रत्यागम info;
    }

    शून्य markClockedWrite(AstVar* varp) { m_clockedWrites.निवेशय(varp); }
    तार्किक hasClockedWrite(AstVar* varp) ध्रुव { प्रत्यागम m_clockedWrites.गणनम्(varp); }

    तार्किक doing_नियोजय() ध्रुव { प्रत्यागम m_doing_नियोजय; }

    ध्रुव VarForceInfo* getVarInfo(AstVarScope* vscp) ध्रुव {
        ध्रुव स्वचालित it = m_var_गन्तव्यम्_Id.find(vscp);
        प्रत्यागम it != m_var_गन्तव्यम्_Id.अन्तः_2() ? &m_varInfos[it->second] : शून्यसूचकः;
    }

    AstVarScope* findScopeVar(AstScope* scopep, ध्रुव AstVar* varp) {
        ScopeVarCache& cache = m_scopeVarCaches[scopep];
        यदि (cache.रिक्तानि()) {
            प्रति (AstVarScope* vscp = scopep->varsp(); vscp;
                 vscp = VN_AS(vscp->nextp(), VarScope)) {
                cache.emplace(vscp->varp(), vscp);
            }
        }
        ध्रुव स्वचालित it = cache.find(varp);
        प्रत्यागम it != cache.अन्तः_2() ? it->second : शून्यसूचकः;
    }
    शून्य सङ्कलय_Force_Assignment(AstVar* varp, AstVarScope* vscp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsExprp,
                            Ast_नियोजय_Force* forceStmtp, पूर्णाङ्क rangeLsb, पूर्णाङ्क rangeMsb, पूर्णाङ्क padLsb,
                            पूर्णाङ्क padMsb, तार्किक hasArraySel, तार्किक skipRdRefresh = असत्य) {
        v3Global.समुच्चयः_उपयुङ्क्ते_Force();
        varp->समुच्चयः_Forced_By_संहिता();

        VarForceInfo& info = getOrCreateVarInfo(vscp);
        ध्रुव पूर्णाङ्क forceId = info.m_forces.आकार();
        File_पङ्क्तिः* ध्रुव flp = varp->fileline();
        AstScope* ध्रुव scopep = vscp->scopep();
        // Allocate one force vector per variable, no matter how many individual force
        // statements later target slices/elements of that variable.
        यदि (!info.m_forceVecVscp) {
            AstCDType* ध्रुव forceVecDtypep = नूतन AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(forceVecDtypep);

            AstVar* ध्रुव forceVecVarp
                = नूतन AstVar{flp, VVarType::MEMBER,
                             varp->name() + (m_doing_नियोजय ? "_VassignVec" : "__VforceVec") + "__"
                                 + scopep->nameDotless(),
                             forceVecDtypep};
            forceVecVarp->funcLocal(असत्य);
            forceVecVarp->is_आन्तरिकम्(सत्य);
            varp->सङ्कलय_अग्र_अत्र(forceVecVarp);
            info.m_forceVecVscp = नूतन AstVarScope{flp, scopep, forceVecVarp};
            scopep->सङ्कलय_Varsp(info.m_forceVecVscp);
        }

        स्वचालित pair = info.m_forces.emplace(forceStmtp,
                                          ForceInfo{rangeLsb, rangeMsb, padLsb, padMsb, forceId,
                                                    hasArraySel, शून्यसूचकः, rhsExprp});
        ForceInfo& finfo = pair.first->second;
        finfo.m_isExternal = forceStmtp->user2();
        finfo.m_skipRdRefresh = skipRdRefresh;
        यदि (doing_नियोजय()) {
            std::सदिशम्<AstVar*> depVarps;
            finfo.m_rhsExprp->foreach([&](AstVarRef* ध्रुव refp) {
                यदि (!refp->access().isReadOnly()) प्रत्यागम;
                AstVar* ध्रुव depVarp = refp->varp();
                यदि (depVarp
                    && std::find(depVarps.begin(), depVarps.अन्तः_2(), depVarp) == depVarps.अन्तः_2()) {
                    depVarps.प्रक्षिप_पृष्ठभागः(depVarp);
                }
            });
            प्रति (AstVar* ध्रुव depVarp : depVarps) m_rhs_Dep_गन्तव्यम्_Forces[depVarp].प्रक्षिप_पृष्ठभागः(&finfo);
        }

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    स्थिर शून्य collectArraySelInfo(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ArraySelInfo& info) {
        यदि (ध्रुव स्वचालित* ध्रुव selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = सत्य;
            collectArraySelInfo(selp->fromp(), info);
        } अन्यथा यदि (स्वचालित* ध्रुव selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.प्रक्षिप_पृष्ठभागः(selp);
        } अन्यथा यदि (ध्रुव स्वचालित* ध्रुव memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    स्थिर ArraySelInfo getArraySelInfo(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        प्रत्यागम info;
    }

    स्थिर std::सदिशम्<पूर्णाङ्क> arraySelDimSizes(ध्रुव ArraySelInfo& info) {
        std::सदिशम्<पूर्णाङ्क> dims;
        dims.reserve(info.m_sels.आकार());
        प्रति (AstArraySel* ध्रुव selp : info.m_sels) {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = selp->fromp()->dtypep()->skipRefp();
            ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.प्रक्षिप_पृष्ठभागः(arrayp->declRange().elements());
        }
        प्रत्यागम dims;
    }

    स्थिर पूर्णाङ्क flatten_सूचकांक(ध्रुव std::सदिशम्<पूर्णाङ्क>& indices, ध्रुव std::सदिशम्<पूर्णाङ्क>& dimSizes) {
        UASSERT(indices.आकार() == dimSizes.आकार(), "Array index and dimension size mismatch");
        पूर्णाङ्क सूचकांक = 0;
        पूर्णाङ्क stride = 1;
        प्रति (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(indices.आकार()) - 1; i >= 0; --i) {
            सूचकांक += indices[i] * stride;
            stride *= dimSizes[i];
        }
        प्रत्यागम सूचकांक;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* build_Flatten_सूचकांक_व्यञ्जकम्(File_पङ्क्तिः* flp, ध्रुव ArraySelInfo& info) {
        ध्रुव std::सदिशम्<पूर्णाङ्क> dimSizes = arraySelDimSizes(info);
        तार्किक allConst = सत्य;
        प्रति (AstArraySel* ध्रुव selp : info.m_sels) {
            यदि (!VN_IS(selp->bitp(), Const)) {
                allConst = असत्य;
                भङ्ग;
            }
        }
        यदि (allConst) {
            std::सदिशम्<पूर्णाङ्क> constIndices;
            constIndices.reserve(info.m_sels.आकार());
            प्रति (AstArraySel* ध्रुव selp : info.m_sels) {
                constIndices.प्रक्षिप_पृष्ठभागः(VN_AS(selp->bitp(), Const)->गन्तव्यम्_SInt());
            }
            प्रत्यागम makeConst32(flp, flatten_सूचकांक(constIndices, dimSizes));
        }
        // A read may select the element at run time, so compute the same flattened index
        // as flattenIndex() does, but as an expression.  Only a force target has to be a
        // constant element; 'array[i]' with a variable 'i' is an ordinary read.
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = शून्यसूचकः;
        पूर्णाङ्क stride = 1;
        प्रति (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(info.m_sels.आकार()) - 1; i >= 0; --i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* termp = info.m_sels[i]->bitp()->clone_वृक्षः_विशुद्धम्(असत्य);
            // V3Width sizes an array index to at most 32 bits, so widening is all that
            // is needed to keep the arithmetic below width matched.
            यदि (termp->विस्तारः() < 32) termp = नूतन AstExtend{flp, termp, 32};
            यदि (stride != 1) termp = नूतन AstMul{flp, termp, makeConst32(flp, stride)};
            resultp = resultp ? नूतन Ast_सङ्कलय{flp, resultp, termp} : termp;
            stride *= dimSizes[i];
        }
        प्रत्यागम resultp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* build_Rhs_दत्त_व्यञ्जकम्(File_पङ्क्तिः* flp, ध्रुव ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        प्रत्यागम नूतन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    शून्य finalizeRhsVars() {
        प्रति (VarForceInfo& info : m_varInfos) {
            AstVar* ध्रुव varp = info.m_varp;
            यदि (info.m_forces.रिक्तानि()) अनुवर्त;

            AstScope* ध्रुव scopep = info.m_scopep;
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            File_पङ्क्तिः* ध्रुव flp = varp->fileline();
            ध्रुव std::सदिशम्<ForceInfo*> forceps = force_Infos_अन्तर्_Id_क्रमः(info);

            प्रति (ForceInfo* ध्रुव finfop : forceps) {
                ForceInfo& finfo = *finfop;
                UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing RHS expression for ForceInfo");

                // Create per-force temporary storage for the captured RHS value.
                AstVar* ध्रुव rhsVarp = नूतन AstVar{
                    flp, VVarType::VAR,
                    varp->name() + (doing_नियोजय() ? "_VassignRHS" : "__VforceRHS")
                        + std::गन्तव्यम्_string(finfo.m_forceId) + "__" + scopep->nameDotless(),
                    finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(सत्य);
                rhsVarp->sig_लौकिकम्(सत्य);
                rhsVarp->समुच्चयः_Forced_By_संहिता();
                varp->सङ्कलय_अग्र_अत्र(rhsVarp);
                finfo.m_rhsVarVscp = नूतन AstVarScope{flp, scopep, rhsVarp};
                scopep->सङ्कलय_Varsp(finfo.m_rhsVarVscp);

                // Array reads use the force vector even when a public read shadow exists.
                // Keep their consumers ordered after RHS capture through forceVec.touch().
                // always_comb begin
                //   forceRHS[id] = rhsExpr;
                //   forceVec.touch();  // When reads use the force vector
                //अन्तः
                Ast_नियोजय* ध्रुव rhsAssignp = नूतन Ast_नियोजय{
                    flp, नूतन AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};

                यदि (!info.m_forceRdVscp || isUnpackedArrayDType(varp->dtypep())) {
                    // touch() is intentionally a semantic no-op at runtime: it creates an
                    // explicit use/ordering edge from the RHS-capture logic to the force vector
                    // so later optimization/scheduling passes keep this update path connected.
                    AstCMethodHard* ध्रुव touchCallp = नूतन AstCMethodHard{
                        flp, नूतन AstVarRef{flp, info.m_forceVecVscp, VAccess::WRITE},
                        VCMethod::FORCE_TOUCH};
                    touchCallp->dtype_समुच्चयः_Void();
                    Ast_ग्रन्थिः_Stmt* ध्रुव touchStmtp = touchCallp->makeStmt();
                    rhsAssignp->सङ्कलय_अग्र_अत्र(touchStmtp);
                }

                // Run both updates in a combinational always block.
                AstAlways* ध्रुव alwaysp
                    = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, शून्यसूचकः, rhsAssignp};
                Ast_Sen_वृक्षः* ध्रुव senTreep
                    = नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Combo{}}};
                Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "force-rhs-update", senTreep};
                activep->sen_वृक्षः_Storep(activep->sentreep());
                activep->सङ्कलय_Stmtsp(alwaysp);
                scopep->सङ्कलय_Blocksp(activep);
            }

            यदि (info.m_forceRdVscp) {
                यदि (!m_forceRdUpdateBuilt.निवेशय(info.m_varVscp).second) अनुवर्त;
                Ast_सक्रिय* ध्रुव सक्रिय_Initp = नूतन Ast_सक्रिय{
                    flp, "force-init",
                    नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Static{}}}};
                सक्रिय_Initp->sen_वृक्षः_Storep(सक्रिय_Initp->sentreep());
                Ast_ग्रन्थिः_Stmt* initStmtp = शून्यसूचकः;
                यदि (AstUnpackArrayDType* ध्रुव arrDtypep
                    = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType)) {
                    ध्रुव std::सदिशम्<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
                    ध्रुव पूर्णाङ्क inner_विस्तारः = dims.पृष्ठभागः()->subDTypep()->skipRefp()->विस्तारः();
                    initStmtp = foreach_Unpacked_पर्णम्(
                        dims, [&](ध्रुव std::सदिशम्<पूर्णाङ्क>& idx, पूर्णाङ्क /*flat*/) -> Ast_ग्रन्थिः_Stmt* {
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = buildNestedArraySel(
                                flp, नूतन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE}, idx);
                            प्रत्यागम नूतन Ast_नियोजय{flp, lhsp, makeZeroConst(varp, inner_विस्तारः)};
                        });
                } अन्यथा {
                    initStmtp = नूतन Ast_नियोजय{
                        flp, नूतन AstVarRef{flp, info.m_forceEnVscp, VAccess::WRITE},
                        makeZeroConst(varp, info.m_forceEnVscp->विस्तारः())};
                }
                initStmtp->सङ्कलय_अग्र(createForceRdUpdateStmt(info));
                सक्रिय_Initp->सङ्कलय_Stmtsp(नूतन AstInitial{flp, initStmtp});
                scopep->सङ्कलय_Blocksp(सक्रिय_Initp);

                AstSenItem* itemsp = शून्यसूचकः;
                स्वचालित सङ्कलय_Sen_Item = [&](AstVarScope* vscp) {
                    यदि (!vscp) प्रत्यागम;
                    AstSenItem* ध्रुव nextp = नूतन AstSenItem{
                        flp, VEdgeType::ET_परिवर्तितम्, नूतन AstVarRef{flp, vscp, VAccess::READ}};
                    यदि (itemsp) {
                        itemsp->सङ्कलय_अग्र(nextp);
                    } अन्यथा {
                        itemsp = nextp;
                    }
                };
                सङ्कलय_Sen_Item(info.m_forceEnVscp);
                सङ्कलय_Sen_Item(info.m_forceValVscp);
                AstVarRef* ध्रुव origSenRefp = नूतन AstVarRef{flp, info.m_varVscp, VAccess::READ};
                markPermanentlyProtected(origSenRefp);
                AstSenItem* ध्रुव origItemp
                    = नूतन AstSenItem{flp, VEdgeType::ET_परिवर्तितम्, origSenRefp};
                यदि (!itemsp) varp->v3fatalSrc("force-rd-update missing force-enable sen item");
                itemsp->सङ्कलय_अग्र(origItemp);
                प्रति (ForceInfo* ध्रुव finfop : forceps) {
                    यदि (!finfop->m_isExternal) सङ्कलय_Sen_Item(finfop->m_rhsVarVscp);
                }

                Ast_सक्रिय* ध्रुव activep
                    = नूतन Ast_सक्रिय{flp, "force-rd-update", नूतन Ast_Sen_वृक्षः{flp, itemsp}};
                activep->sen_वृक्षः_Storep(activep->sentreep());
                activep->सङ्कलय_Stmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, शून्यसूचकः,
                                                 createForceRdUpdateStmt(info)});
                scopep->सङ्कलय_Blocksp(activep);
            }
        }
    }

    Ast_ग्रन्थिः* createRhsUpdatesForWrite(File_पङ्क्तिः* flp, AstVar* writtenVarp) ध्रुव {
        यदि (!doing_नियोजय()) प्रत्यागम शून्यसूचकः;

        ध्रुव स्वचालित it = m_rhs_Dep_गन्तव्यम्_Forces.find(writtenVarp);
        यदि (it == m_rhs_Dep_गन्तव्यम्_Forces.अन्तः_2()) प्रत्यागम शून्यसूचकः;

        Ast_ग्रन्थिः* headp = शून्यसूचकः;
        Ast_ग्रन्थिः* tailp = शून्यसूचकः;
        प्रति (ध्रुव ForceInfo* ध्रुव finfop : it->second) {
            UASSERT_OBJ(finfop->m_rhsVarVscp, writtenVarp, "No RHS var for forced variable");
            UASSERT_OBJ(finfop->m_rhsExprp, writtenVarp, "Missing RHS expression");
            Ast_नियोजय* ध्रुव updatep
                = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, finfop->m_rhsVarVscp, VAccess::WRITE},
                                finfop->m_rhsExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
            यदि (tailp) {
                tailp->सङ्कलय_अग्र_अत्र(updatep);
            } अन्यथा {
                headp = updatep;
            }
            tailp = updatep;
        }
        प्रत्यागम headp;
    }

    ध्रुव ForceInfo& getForceInfo(Ast_नियोजय_Force* forceStmtp) ध्रुव {
        AstVarScope* ध्रुव vscp = getOneVarRef(forceStmtp->lhsp())->varScopep();
        ध्रुव VarForceInfo* ध्रुव varInfo = getVarInfo(vscp);
        UASSERT(varInfo, "Force info not found for variable");
        स्वचालित it2 = varInfo->m_forces.find(forceStmtp);
        UASSERT(it2 != varInfo->m_forces.अन्तः_2(), "Force statement not found");
        प्रत्यागम it2->second;
    }

    स्थिर तार्किक selOverlapsAnyForce(ध्रुव VarForceInfo& varInfo, पूर्णाङ्क selLsb, पूर्णाङ्क selMsb) {
        प्रति (ध्रुव स्वचालित& pair : varInfo.m_forces) {
            यदि (pair.second.m_rangeLsb <= selMsb && pair.second.m_rangeMsb >= selLsb) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createForceReadExpression(ध्रुव VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) ध्रुव {
        File_पङ्क्तिः* ध्रुव flp = originalRefp->fileline();
        AstVarRef* ध्रुव refp = originalRefp->clone_वृक्षः_विशुद्धम्(असत्य);
        refp->access(VAccess::READ);
        प्रत्यागम createForceReadCall(varInfo, flp, VCMethod::FORCE_READ, refp, refp->varp(),
                                   शून्यसूचकः);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* create_Force_Read_सूचकांक_Expression(ध्रुव VarForceInfo& varInfo,
                                                Ast_ग्रन्थिः_व्यञ्जकम्* originalExprp,
                                                Ast_ग्रन्थिः_व्यञ्जकम्* सूचकांक_Exprp) ध्रुव {
        File_पङ्क्तिः* ध्रुव flp = originalExprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = originalExprp->clone_वृक्षः_विशुद्धम्(असत्य);
        // Must be an LValue to a static variable
        VN_AS(exprp->getVAccessTargetRecurse(), VarRef)->access(VAccess::READ);
        प्रत्यागम createForceReadCall(varInfo, flp, VCMethod::FORCE_READ_सूचकांक, exprp, originalExprp,
                                   सूचकांक_Exprp);
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* rebuild_Sel_मार्गः(Ast_ग्रन्थिः_व्यञ्जकम्* pathp, Ast_ग्रन्थिः_व्यञ्जकम्* आधारः_Exprp) {
        यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(pathp, Sel)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = rebuild_Sel_मार्गः(selp->fromp(), आधारः_Exprp);
            AstSel* ध्रुव outp
                = नूतन AstSel{selp->fileline(), fromp, selp->lsbConst(), selp->विस्तारः_Const()};
            outp->dtypeFrom(selp);
            प्रत्यागम outp;
        }
        प्रत्यागम आधारः_Exprp;
    }
};

// Split deassign concat LHS before converting to release internals.
स्थिर शून्य विभज_Deassign(AstDeassign* nodep) {
    AstConcat* ध्रुव concatp = VN_CAST(nodep->lhsp(), Concat);
    यदि (!concatp) प्रत्यागम;

    File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
    AstDeassign* ध्रुव newLp = नूतन AstDeassign{flp, concatp->lhsp()->unlink_Fr_पृष्ठभागः()};
    AstDeassign* ध्रुव newRp = नूतन AstDeassign{flp, concatp->rhsp()->unlink_Fr_पृष्ठभागः()};
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conp = concatp->unlink_Fr_पृष्ठभागः();
    nodep->replaceWith(newLp);
    newLp->सङ्कलय_अग्र_अत्र(newRp);
    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    VL_DO_DANGLING(conp->delete_वृक्षः(), conp);

    विभज_Deassign(newLp);
    विभज_Deassign(newRp);
}

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

वर्ग ForceDiscoveryVisitor final : सार्वजनिक VNVisitorConst {
    ForceState& m_state;
    तार्किक m_अन्तर्_Clocked_सक्रिय = असत्य;

    शून्य buildForceableUnpackedArray(AstVarScope* ध्रुव nodep,
                                     AstUnpackArrayDType* ध्रुव arrDtypep) {
        AstVar* ध्रुव varp = nodep->varp();
        ध्रुव std::सदिशम्<AstUnpackArrayDType*> dims = arrDtypep->unpackDimensions();
        UASSERT_OBJ(!dims.रिक्तानि(), varp,
                    "buildForceableUnpackedArray called with non-unpacked dtype");
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव पर्णम्_Dtypep = dims.पृष्ठभागः()->subDTypep()->skipRefp();
        ध्रुव AstBasicDType* ध्रुव innerBasicp = पर्णम्_Dtypep->basicp();
        यदि (!innerBasicp || !ForceState::isBitwiseDType(innerBasicp)) {
            varp->v3warn(E_UNSUPPORTED,
                         "Unsupported: Forcing unpacked arrays of non-bitwise inner type: "
                             << varp->name());  // (#4735)
            प्रत्यागम;
        }

        File_पङ्क्तिः* ध्रुव flp = varp->fileline();
        ध्रुव पूर्णाङ्क inner_विस्तारः = पर्णम्_Dtypep->विस्तारः();

        ForceState::VarForceInfo& info = m_state.getOrCreateVarInfo(nodep);
        AstVarScope* ध्रुव enVscp = info.m_forceEnVscp;
        AstVarScope* ध्रुव valVscp = info.m_forceValVscp;

        AstSenItem* ध्रुव itemsp = नूतन AstSenItem{flp, VEdgeType::ET_परिवर्तितम्,
                                                  नूतन AstVarRef{flp, enVscp, VAccess::READ}};
        Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "force-update", नूतन Ast_Sen_वृक्षः{flp, itemsp}};
        activep->sen_वृक्षः_Storep(activep->sentreep());

        Ast_ग्रन्थिः_Stmt* ध्रुव alwaysBodyHeadp = ForceState::foreach_Unpacked_पर्णम्(
            dims, [&](ध्रुव std::सदिशम्<पूर्णाङ्क>& idx, पूर्णाङ्क flat) -> Ast_ग्रन्थिः_Stmt* {
                AstVarRef* ध्रुव origRefp = नूतन AstVarRef{flp, nodep, VAccess::READ};
                ForceState::markNonReplaceable(origRefp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origSelp = ForceState::buildNestedArraySel(flp, origRefp, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, enVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, valVscp, VAccess::READ}, idx);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव forceExprp = नूतन AstOr{
                    flp, नूतन AstAnd{flp, enSelp, valSelp},
                    नूतन AstAnd{flp, नूतन AstNot{flp, enSelp->clone_वृक्षः_विशुद्धम्(असत्य)}, origSelp}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsSelp = ForceState::buildNestedArraySel(
                    flp, नूतन AstVarRef{flp, nodep, VAccess::WRITE}, idx);

                Ast_नियोजय_Force* ध्रुव forceAssignp = नूतन Ast_नियोजय_Force{flp, lhsSelp, forceExprp};
                forceAssignp->user2(सत्य);

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsClonep = forceExprp->clone_वृक्षः_विशुद्धम्(असत्य);
                rhsClonep->foreach([varp](AstVarRef* ध्रुव r) {
                    यदि (r->varp() == varp) ForceState::markNonReplaceable(r);
                });
                m_state.सङ्कलय_Force_Assignment(varp, nodep, rhsClonep, forceAssignp,
                                           /*rangeLsb=*/flat, /*rangeMsb=*/flat,
                                           /*padLsb=*/0, /*padMsb=*/inner_विस्तारः - 1,
                                           /*hasArraySel=*/सत्य, /*skipRdRefresh=*/सत्य);
                प्रत्यागम forceAssignp;
            });
        activep->सङ्कलय_Stmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, शून्यसूचकः, alwaysBodyHeadp});
        nodep->scopep()->सङ्कलय_Blocksp(activep);
    }

    शून्य दर्शय(Ast_नियोजय_Force* nodep) override {
        यदि (nodep->user2()) प्रत्यागम;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* ध्रुव lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* ध्रुव forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        // Resolve force bookkeeping range/padding for the LHS shape.
        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, सत्य);

        // Keep narrow rhs, VlForceVec blends unpacked-array bit-select forces at read time
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsExprp = nodep->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य);

        m_state.सङ्कलय_Force_Assignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
    }

    शून्य दर्शय(Ast_नियोजय* nodep) override {
        यदि (m_state.doing_नियोजय() && m_अन्तर्_Clocked_सक्रिय) {
            यदि (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                m_state.markClockedWrite(lhsp->varp());
            }
        }
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_सक्रिय* nodep) override {
        VL_RESTORER(m_अन्तर्_Clocked_सक्रिय);
        m_अन्तर्_Clocked_सक्रिय = nodep->hasClocked();
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstVarScope* nodep) override {
        यदि (nodep->varp()->isForceable()) {
            // assignAll() runs after forceAll() and traverses the same netlist with a fresh
            // ForceState. Reuse already-created public helper vars instead of regenerating
            // duplicate __Vforce* members for every forceable signal.
            यदि (m_state.doing_नियोजय()) {
                m_state.getOrCreateVarInfo(nodep);
                iterateChildrenConst(nodep);
                प्रत्यागम;
            }

            यदि (AstUnpackArrayDType* ध्रुव arrDtypep
                = VN_CAST(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                buildForceableUnpackedArray(nodep, arrDtypep);
                iterateChildrenConst(nodep);
                प्रत्यागम;
            }

            ध्रुव AstBasicDType* ध्रुव bdtypep = nodep->varp()->basicp();
            यदि (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
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
            AstSenItem* ध्रुव itemsp = नूतन AstSenItem{flp, VEdgeType::ET_परिवर्तितम्,
                                                      नूतन AstVarRef{flp, enVscp, VAccess::READ}};
            Ast_सक्रिय* ध्रुव activep
                = नूतन Ast_सक्रिय{flp, "force-update", नूतन Ast_Sen_वृक्षः{flp, itemsp}};
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
                            नूतन AstAnd{flp, नूतन AstNot{flp, enRefp->clone_वृक्षः_विशुद्धम्(असत्य)}, refp}})
                      : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstCond{flp, enRefp, valRefp, refp});
            Ast_नियोजय_Force* ध्रुव forceAssignp
                = नूतन Ast_नियोजय_Force{flp, नूतन AstVarRef{flp, nodep, VAccess::WRITE}, forceExprp};
            forceAssignp->user2(सत्य);
            activep->सङ्कलय_Stmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, शून्यसूचकः, forceAssignp});
            nodep->scopep()->सङ्कलय_Blocksp(activep);

            // Clone the RHS for tracking and preserve original var refs as non-replaceable.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsClonep = forceExprp->clone_वृक्षः_विशुद्धम्(असत्य);
            rhsClonep->foreach([varp](AstVarRef* ध्रुव refp) {
                यदि (refp->varp() == varp) ForceState::markNonReplaceable(refp);
            });

            // Compute full assignment range (including unpacked arrays) for force bookkeeping.
            ध्रुव तार्किक bitwiseVar = ForceState::isBitwiseDType(varp);
            ध्रुव पूर्णाङ्क padMsb = bitwiseVar ? (varp->विस्तारः() - 1) : 0;
            पूर्णाङ्क rangeLsb = 0;
            पूर्णाङ्क rangeMsb = padMsb;
            यदि (ForceState::isUnpackedArrayDType(varp->dtypep())) {
                nodep->v3fatalSrc("Forceable unpacked arrays should have been rejected earlier");
            }
            m_state.सङ्कलय_Force_Assignment(varp, nodep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0,
                                       padMsb, असत्य);
        }
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्पष्ट ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_अग्र_Const_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

वर्ग ForceConvertVisitor final : सार्वजनिक VNVisitor {
    ForceState& m_state;

    शून्य दर्शय(Ast_नियोजय_Force* nodep) override {
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
        Ast_नियोजय* ध्रुव rhsAssignp
            = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य)};

        Ast_नियोजय* valAssignp = शून्यसूचकः;
        Ast_नियोजय* enAssignp = शून्यसूचकः;
        ध्रुव तार्किक bitwiseForcedVar = ForceState::isBitwiseDType(forcedVarp);
        // When an externally forceable signal is also forced in (System)Verilog code
        // keep the public __VforceEn/__VforceVal signals in sync with the procedural force too.
        // Don't do this for array selections; those are represented only in VlForceVec.
        यदि (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            Ast_ग्रन्थिः_व्यञ्जकम्* आधारः_Exprp = nodep->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य);
            आधारः_Exprp->foreach(
                [](AstVarRef* ध्रुव refp) { ForceState::markNonReplaceable(refp); });
            यदि (bitwiseForcedVar) {
                आधारः_Exprp = ForceState::zero_Pad_गन्तव्यम्_आधारः_विस्तारः(आधारः_Exprp, forcedVarp->विस्तारः(),
                                                           info.m_padLsb, info.m_padMsb);
            }
            यदि (bitwiseForcedVar) {
                // forceVal = (forceVal & ~mask(range)) | (rhs_padded & mask(range));
                // forceEn  = forceEn | mask(range);
                AstConst* ध्रुव आवरणम्_Constp = ForceState::make_Range_आवरणम्_Const(
                    nodep, forcedVarp->विस्तारः(), info.m_rangeLsb, info.m_rangeMsb);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valReadp
                    = नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valWritep
                    = नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notMaskp = नूतन AstNot{flp, आवरणम्_Constp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maskedOldp = नूतन AstAnd{flp, valReadp, notMaskp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newValp = नूतन AstOr{flp, maskedOldp, आधारः_Exprp};
                valAssignp = नूतन Ast_नियोजय{flp, valWritep, newValp};

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enReadp
                    = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enWritep
                    = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newEnp
                    = नूतन AstOr{flp, enReadp, आवरणम्_Constp->clone_वृक्षः_विशुद्धम्(असत्य)};
                enAssignp = नूतन Ast_नियोजय{flp, enWritep, newEnp};
            } अन्यथा {
                AstConst* ध्रुव oneConstp = ForceState::make_Range_आवरणम्_Const(nodep, 1, 0, 0);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsValp = ForceState::cast_गन्तव्यम्_ग्रन्थिः_DType(आधारः_Exprp, forcedVarp);
                valAssignp = नूतन Ast_नियोजय{
                    flp, नूतन AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = नूतन Ast_नियोजय{
                    flp, नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Verilog pseudocode:
        //   forceVec.addForce(range_lsb, range_msb, &forceRHS[id], rhs_lsb);
        ध्रुव AstSel* ध्रुव selLhsp = VN_CAST(lhsp, Sel);
        ध्रुव तार्किक arrayBitSel
            = info.m_hasArraySel && selLhsp && ForceState::getArraySelInfo(lhsp).m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (info.m_padMsb - info.m_padLsb + 1) < selLhsp->fromp()->विस्तारः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsDatap = ForceState::build_Rhs_दत्त_व्यञ्जकम्(flp, info);
        AstCExpr* ध्रुव rhsAddrp = नूतन AstCExpr{flp};
        rhsAddrp->सङ्कलय("&(");
        rhsAddrp->सङ्कलय(rhsDatap);
        rhsAddrp->सङ्कलय(")");
        AstCMethodHard* ध्रुव सङ्कलय_Force_Callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE}, VCMethod::FORCE_सङ्कलय,
            ForceState::makeConst32(flp, info.m_rangeLsb)};
        सङ्कलय_Force_Callp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, info.m_rangeMsb));
        सङ्कलय_Force_Callp->सङ्कलय_Pinsp(rhsAddrp);
        सङ्कलय_Force_Callp->सङ्कलय_Pinsp(
            ForceState::makeConst32(flp, arrayBitSel ? info.m_padLsb : info.m_rangeLsb));
        यदि (arrayBitSel) {
            सङ्कलय_Force_Callp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, info.m_padLsb));
            सङ्कलय_Force_Callp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, info.m_padMsb));
            सङ्कलय_Force_Callp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, selLhsp->fromp()->विस्तारः()));
        }
        सङ्कलय_Force_Callp->dtype_समुच्चयः_Void();
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = सङ्कलय_Force_Callp->makeStmt();

        Ast_ग्रन्थिः* tailp = rhsAssignp;
        यदि (valAssignp) {
            tailp->सङ्कलय_अग्र_अत्र(valAssignp);
            tailp = valAssignp;
        }
        यदि (enAssignp) {
            tailp->सङ्कलय_अग्र_अत्र(enAssignp);
            tailp = enAssignp;
        }
        tailp->सङ्कलय_अग्र_अत्र(stmtp);
        यदि (varInfo->m_forceRdVscp && !info.m_skipRdRefresh) {
            stmtp->सङ्कलय_अग्र_अत्र(m_state.createForceRdUpdateStmt(*varInfo));
        }
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        AstVarRef* ध्रुव lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* ध्रुव releasedVarp = lhsVarRefp->varp();

        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo
            = m_state.getVarInfo(lhsVarRefp->varScopep());
        यदि (!varInfo) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        UASSERT_OBJ(!varInfo->m_forces.रिक्तानि(), nodep, "Var info for variable with no forces");

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        ध्रुव ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, असत्य);

        ध्रुव AstSel* ध्रुव selLhsp = VN_CAST(lhsp, Sel);
        ध्रुव तार्किक arrayBitSel
            = rangeInfo.m_hasArraySel && selLhsp && rangeInfo.m_arrayInfo.m_hasBitSel
              && ForceState::isBitwiseDType(selLhsp->fromp())
              && (rangeInfo.m_padMsb - rangeInfo.m_padLsb + 1) < selLhsp->fromp()->विस्तारः();
        AstCMethodHard* ध्रुव releaseCallp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE},
            VCMethod::FORCE_RELEASE, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb)};
        releaseCallp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_rangeMsb));
        यदि (arrayBitSel) {
            releaseCallp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padLsb));
            releaseCallp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, rangeInfo.m_padMsb));
        }
        releaseCallp->dtype_समुच्चयः_Void();
        // forceVec.release(range_lsb, range_msb [, bit_lsb, bit_msb]);
        Ast_ग्रन्थिः_Stmt* ध्रुव releasep = releaseCallp->makeStmt();

        Ast_नियोजय* clearEnp = शून्यसूचकः;
        // Releases must also clear the external/public force-enable, but only for
        // directly forceable variables and only for non-array-select cases that use that external
        // force.
        यदि (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enWritep
                = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            यदि (ForceState::isBitwiseDType(releasedVarp)) {
                ध्रुव पूर्णाङ्क var_विस्तारः = releasedVarp->विस्तारः();
                यदि (rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == var_विस्तारः - 1) {
                    clearEnp
                        = नूतन Ast_नियोजय{flp, enWritep, ForceState::makeZeroConst(nodep, var_विस्तारः)};
                } अन्यथा {
                    // forceEn = forceEn & ~mask(range);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enReadp
                        = नूतन AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                    AstConst* ध्रुव आवरणम्_Const = ForceState::make_Range_आवरणम्_Const(
                        nodep, var_विस्तारः, rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newEnp
                        = नूतन AstAnd{flp, enReadp, नूतन AstNot{flp, आवरणम्_Const}};
                    clearEnp = नूतन Ast_नियोजय{flp, enWritep, newEnp};
                }
            } अन्यथा {
                clearEnp = नूतन Ast_नियोजय{flp, enWritep, ForceState::makeZeroConst(nodep, 1)};
            }
        }

        ध्रुव AstSel* ध्रुव selp = VN_CAST(lhsp, Sel);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव basep = selp ? selp->fromp() : lhsp;

        Ast_ग्रन्थिः* stmtListp = releasep;
        यदि (clearEnp) {
            clearEnp->सङ्कलय_अग्र_अत्र(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        ध्रुव तार्किक पूर्णम्_Bitwise_Release
            = ForceState::isBitwiseDType(releasedVarp) && !rangeInfo.m_hasArraySel && !selp
              && rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == releasedVarp->विस्तारः() - 1;
        यदि (!releasedVarp->isContinuously()
            && !(m_state.doing_नियोजय() && m_state.hasClockedWrite(releasedVarp)
                 && पूर्णम्_Bitwise_Release)) {
            // Member/struct paths on non-bitwise types do not lower to a plain VarRef/bit range,
            // so their current forced value is recovered via the same synthetic path index.
            // if (!continuously_driven) lhs = force_read_current(lhs_path);
            // forceVec.release(range);
            ध्रुव तार्किक has_Opaque_मार्गः = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            Ast_ग्रन्थिः_व्यञ्जकम्* forceReadp = शून्यसूचकः;
            यदि (has_Opaque_मार्गः) {
                forceReadp = m_state.create_Force_Read_सूचकांक_Expression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } अन्यथा यदि (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.create_Force_Read_सूचकांक_Expression(
                    *varInfo, basep,
                    ForceState::build_Flatten_सूचकांक_व्यञ्जकम्(flp, rangeInfo.m_arrayInfo));
                यदि (selp) { forceReadp = ForceState::rebuild_Sel_मार्गः(lhsp, forceReadp); }
            } अन्यथा {
                forceReadp
                    = selp ? ForceState::rebuild_Sel_मार्गः(
                                 lhsp, m_state.createForceReadExpression(*varInfo, lhsVarRefp))
                           : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            }
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{flp, lhsp->clone_वृक्षः_विशुद्धम्(असत्य), forceReadp};
            assignp->सङ्कलय_अग्र_अत्र(stmtListp);
            stmtListp = assignp;
        }

        यदि (varInfo->m_forceRdVscp) stmtListp->सङ्कलय_अग्र(m_state.createForceRdUpdateStmt(*varInfo));

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterate_And_अग्र_Null(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

वर्ग ForceReplaceVisitor final : सार्वजनिक VNVisitor {
    ध्रुव ForceState& m_state;
    VDouble0 m_nonOverlappingForceSels;  // Statistic tracking
    Ast_ग्रन्थिः_Stmt* m_stmtp = शून्यसूचकः;
    तार्किक m_अन्तर्_Logic = असत्य;

    शून्य iterateLogic(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_अन्तर्_Logic);
        m_अन्तर्_Logic = सत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_नियोजय* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate(nodep->lhsp());
        iterate(nodep->rhsp());
        यदि (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp()->आधारः_Fromp(सत्य), VarRef)) {
            यदि (Ast_ग्रन्थिः* ध्रुव updatep
                = m_state.createRhsUpdatesForWrite(nodep->fileline(), lhsp->varp())) {
                nodep->सङ्कलय_अग्र_अत्र(updatep);
            }
        }
    }
    शून्य दर्शय(Ast_नियोजय_Cont* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterate_And_अग्र_Null(nodep->timingControlp());
        iterate(nodep->rhsp());
    }
    शून्य दर्शय(AstDeassign*) override {}
    शून्य दर्शय(AstCFunc* nodep) override { iterateLogic(nodep); }
    शून्य दर्शय(AstCoverToggle* nodep) override { iterateLogic(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override { iterateLogic(nodep); }
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (nodep->keyword() == VAlwaysKwd::CONT_नियोजय) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        iterateLogic(nodep);
    }
    शून्य दर्शय(AstSenItem* nodep) override { iterateLogic(nodep); }
    शून्य दर्शय(AstSel* nodep) override {
        // Replace Sel on a wide with readSelI/Q/W to avoid materializing the full value
        AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef);
        यदि (!refp || m_state.isNotReplaceable(refp) || !refp->access().isReadOnly()) {
            दर्शय(static_cast<Ast_ग्रन्थिः*>(nodep));
            प्रत्यागम;
        }

        AstVar* ध्रुव varp = refp->varp();
        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo = m_state.getVarInfo(refp->varScopep());
        यदि (!varInfo || varInfo->m_forceRdVscp || varInfo->m_forces.रिक्तानि()
            || !ForceState::isBitwiseDType(varp) || !varp->dtypep()->isWide()) {
            दर्शय(static_cast<Ast_ग्रन्थिः*>(nodep));
            प्रत्यागम;
        }

        यदि (ध्रुव AstConst* ध्रुव lsbConstp = VN_CAST(nodep->lsbp(), Const)) {
            ध्रुव पूर्णाङ्क selLsb = lsbConstp->गन्तव्यम्_SInt();
            ध्रुव पूर्णाङ्क selMsb = selLsb + nodep->विस्तारः() - 1;
            यदि (!varp->is_Sig_लौकिकम्()
                && !ForceState::selOverlapsAnyForce(*varInfo, selLsb, selMsb)) {
                m_nonOverlappingForceSels++;
                ForceState::markNonReplaceable(refp);
                दर्शय(static_cast<Ast_ग्रन्थिः*>(nodep));
                प्रत्यागम;
            }
        }

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ForceState::markNonReplaceable(refp);
        AstVarRef* ध्रुव refClonep = refp->clone_वृक्षः_विशुद्धम्(असत्य);
        ForceState::markNonReplaceable(refClonep);
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::READ},
            VCMethod::FORCE_READ_SEL, ForceState::makeConst32(flp, varp->विस्तारः())};
        callp->सङ्कलय_Pinsp(refClonep);
        callp->सङ्कलय_Pinsp(nodep->lsbp()->clone_वृक्षः_विशुद्धम्(असत्य));
        callp->सङ्कलय_Pinsp(ForceState::makeConst32(flp, nodep->विस्तारः()));
        callp->dtypeFrom(nodep);
        nodep->replaceWith(callp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstArraySel* nodep) override {
        यदि (ध्रुव AstArraySel* ध्रुव पृष्ठभागः_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            // Only the outermost selection of the array path should become a force-aware
            // read; inner selections along 'fromp' fold into the final flattened index.
            // A selection used as the index is a read in its own right, as in
            // 'mem[mem[0]]', so it must not be skipped here.
            यदि (पृष्ठभागः_Selp->fromp() == nodep) {
                iterateChildren(nodep);
                प्रत्यागम;
            }
        }

        Ast_ग्रन्थिः* ध्रुव basep = nodep->आधारः_Fromp(सत्य);
        AstVarRef* ध्रुव आधारः_Refp = VN_CAST(basep, VarRef);
        यदि (!आधारः_Refp) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        AstVar* ध्रुव varp = आधारः_Refp->varp();
        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo = m_state.getVarInfo(आधारः_Refp->varScopep());
        // Skip non-forceable reads, reads we intentionally protected earlier, and intermediate
        // selections that still evaluate to an unpacked array rather than a scalar element.
        यदि (m_state.isNotReplaceable(आधारः_Refp) || !varInfo
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            प्रत्यागम;
        }

        यदि (!आधारः_Refp->access().isReadOnly()) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        यदि (m_state.doing_नियोजय() && varInfo->m_forceRdVscp) {
            आधारः_Refp->varp(varInfo->m_forceRdVscp->varp());
            आधारः_Refp->varScopep(varInfo->m_forceRdVscp);
            iterateChildren(nodep);
            प्रत्यागम;
        }
        ध्रुव ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
        // Substitute forced reads inside the index expressions before anything is cloned,
        // so the fallback value and the flattened index use the same, force-aware index.
        // An index is an ordinary read, including when it reads the same array as in
        // 'mem[mem[0]]'.
        प्रति (AstArraySel* ध्रुव selp : arrayInfo.m_sels) iterate_And_अग्र_Null(selp->bitp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_Exprp
            = ForceState::build_Flatten_सूचकांक_व्यञ्जकम्(nodep->fileline(), arrayInfo);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp
            = m_state.create_Force_Read_सूचकांक_Expression(*varInfo, nodep, सूचकांक_Exprp);
        nodep->replaceWith(readExprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (m_state.isNotReplaceable(nodep)) प्रत्यागम;
        // The array an ArraySel selects from is left to visit(AstArraySel), which builds
        // the force-aware read for the whole select. The index is an ordinary read and
        // must still be substituted here, so check which child this is.
        यदि (ध्रुव AstArraySel* ध्रुव पृष्ठभागः_Selp = VN_CAST(nodep->backp(), ArraySel)) {
            यदि (पृष्ठभागः_Selp->fromp() == nodep) प्रत्यागम;
        }

        ध्रुव ForceState::VarForceInfo* ध्रुव varInfo = m_state.getVarInfo(nodep->varScopep());
        यदि (!varInfo) प्रत्यागम;

        यदि (varInfo->m_forceRdVscp) {
            यदि (nodep->access().isRW()) {
                यदि (m_अन्तर्_Logic) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Signals used via read-write reference cannot be "
                                  "forced");
                }
                प्रत्यागम;
            }
            यदि (nodep->access().isReadOnly()) {
                // Inline the effective value so a blocking update to a continuous driver is
                // visible immediately. The __VforceRd shadow is updated only on the next event.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readp
                    = m_state.createForceRdExpression(*varInfo, nodep->fileline());
                nodep->replaceWith(readp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
            यदि (m_अन्तर्_Logic && m_stmtp) {
                m_stmtp->सङ्कलय_अग्र_अत्र(m_state.createForceRdUpdateStmt(*varInfo));
            }
            प्रत्यागम;
        }

        // For opaque member/struct paths we rewrite the outer path node instead of the base
        // VarRef, so leave the base reference alone and let visit(AstNode*) handle it.
        यदि (nodep->backp() && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            प्रत्यागम;
        }

        UASSERT_OBJ(!varInfo->m_forces.रिक्तानि(), nodep, "Var info for variable with no forces");

        यदि (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } अन्यथा यदि (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (ForceState::is_Outermost_Opaque_मार्गः_Selector(nodep)) {
            // Handle the whole opaque path at its outermost node so we can assign one stable
            // synthetic force-path index to the full selection/member chain.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_AS(nodep, ग्रन्थिः_व्यञ्जकम्);
            Ast_ग्रन्थिः* ध्रुव basep = exprp->आधारः_Fromp(सत्य);
            AstVarRef* ध्रुव आधारः_Refp = VN_CAST(basep, VarRef);
            यदि (आधारः_Refp) {
                AstVar* ध्रुव varp = आधारः_Refp->varp();
                यदि (!ForceState::isBitwiseDType(varp)
                    && !ForceState::isUnpackedArrayDType(varp->dtypep())) {
                    ध्रुव ForceState::VarForceInfo* ध्रुव varInfo
                        = m_state.getVarInfo(आधारः_Refp->varScopep());
                    यदि (!m_state.isNotReplaceable(आधारः_Refp) && varInfo) {
                        ध्रुव पूर्णाङ्क force_मार्गः_सूचकांक = varInfo->find_Force_मार्गः_सूचकांक(exprp);
                        यदि (force_मार्गः_सूचकांक >= 0) {
                            यदि (!आधारः_Refp->access().isReadOnly()) प्रत्यागम;
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readExprp = m_state.create_Force_Read_सूचकांक_Expression(
                                *varInfo, exprp,
                                ForceState::makeConst32(nodep->fileline(), force_मार्गः_सूचकांक));
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
    स्पष्ट ForceReplaceVisitor(AstNetlist* nodep, ध्रुव ForceState& state)
        : m_state{state} {
        iterate_And_अग्र_Null(nodep->modulesp());
    }
    ~ForceReplaceVisitor() override {
        V3Stats::सङ्कलय_Stat("Non-overlapping force sels", m_nonOverlappingForceSels);
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

स्थिर शून्य forceAllImpl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                         ForceState::Protected_Var_Ref_समुच्चयः& permanentlyProtected,
                         ForceState::Built_Var_Scope_समुच्चयः& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    यदि (!v3Global.has_Forceable_सङ्केताः()) प्रत्यागम;
    ForceState state{असत्य, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्षः("force", 0, dump_वृक्षः_Either_Level() >= 3);
}

स्थिर शून्य नियोजय_All_Impl(AstNetlist* nodep, ForceState::ForceHelperVarsByVar& helperVars,
                          ForceState::Protected_Var_Ref_समुच्चयः& permanentlyProtected,
                          ForceState::Built_Var_Scope_समुच्चयः& forceRdUpdateBuilt) {
    UINFO(2, __FUNCTION__ << ":\n");
    यदि (!v3Global.has_नियोजय_Deassign()) प्रत्यागम;

    std::सदिशम्<AstDeassign*> deassignps;
    nodep->foreach([&](AstDeassign* deassignp) { deassignps.प्रक्षिप_पृष्ठभागः(deassignp); });
    प्रति (AstDeassign* ध्रुव deassignp : deassignps) विभज_Deassign(deassignp);

    std::सदिशम्<Ast_नियोजय_Cont*> नियोजय_Contps;
    deassignps.clear();
    nodep->foreach([&](Ast_ग्रन्थिः_Stmt* nodep) {
        यदि (Ast_नियोजय_Cont* ध्रुव नियोजय_Contp = VN_CAST(nodep, नियोजय_Cont)) {
            नियोजय_Contps.प्रक्षिप_पृष्ठभागः(नियोजय_Contp);
        } अन्यथा यदि (AstDeassign* ध्रुव deassignp = VN_CAST(nodep, Deassign)) {
            deassignps.प्रक्षिप_पृष्ठभागः(deassignp);
        }
    });

    प्रति (Ast_नियोजय_Cont* ध्रुव assignp : नियोजय_Contps) {
        assignp->replaceWith(नूतन Ast_नियोजय_Force{assignp->fileline(),
                                                assignp->lhsp()->unlink_Fr_पृष्ठभागः(),
                                                assignp->rhsp()->unlink_Fr_पृष्ठभागः()});
        VL_DO_DANGLING(assignp->delete_वृक्षः(), assignp);
    }
    प्रति (AstDeassign* ध्रुव deassignp : deassignps) {
        deassignp->replaceWith(
            नूतन AstRelease{deassignp->fileline(), deassignp->lhsp()->clone_वृक्षः_विशुद्धम्(सत्य)});
        VL_DO_DANGLING(deassignp->delete_वृक्षः(), deassignp);
    }
    ForceState state{सत्य, helperVars, permanentlyProtected, forceRdUpdateBuilt};
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dump_Check_Global_वृक्षः("assign-deassign", 0, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3Force::force_And_नियोजय_All(AstNetlist* nodep) {
    ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
    ForceState::ForceHelperVarsByVar helperVars;
    ForceState::Protected_Var_Ref_समुच्चयः permanentlyProtected;
    ForceState::Built_Var_Scope_समुच्चयः forceRdUpdateBuilt;
    forceAllImpl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
    नियोजय_All_Impl(nodep, helperVars, permanentlyProtected, forceRdUpdateBuilt);
}
