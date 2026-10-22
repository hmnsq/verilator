// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Parse module/signal name references
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
// Slice TRANSFORMATIONS:
//      Top-down traversal (SliceVisitor):
//        NODEASSIGN
//          ARRAYSEL
//            Compare the dimensions to the Var to check for implicit slices.
//            Using ->length() calculate the number of clones needed.
//          VARREF
//            Check the dimensions of the Var for an implicit slice.
//            Replace with ArraySel nodes if needed.
//          SEL, EXTEND
//            We might be assigning a 1-D packed array to a 2-D packed array,
//            this is unsupported.
//          SliceCloneVisitor (called if this node is a slice):
//            NODEASSIGN
//              Clone and iterate the clone:
//                ARRAYSEL
//                  Modify bitp() for the new value and set ->length(1)
//
// TODO: This code was written before SLICESEL was a type, it might be
// simplified to look primarily for SLICESELs.
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Slice.h"

#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

తరగతి ముక్క_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_stat_ముక్క_Element_Skips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు* m_assignp = శూన్యసూచిక;  // Assignment we are under
    తార్కిక m_కేటాయించు_Error = అబద్ధం;  // True if the current assign already has an error
    తార్కిక m_okConsArray = అబద్ధం;  // Allow ConsArray children

    // METHODS
    Ast_కణుపు_సమాసం* cloneAndSel(Ast_కణుపు_సమాసం* స్థిరరాశి nodep, uint64_t elements, uint64_t elemIdx,
                             స్థిరరాశి తార్కిక need_శుద్ధ) {
        // Insert an ArraySel, except for a few special cases
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ఒకవేళ (!arrayp) {  // V3Width should have complained, but...
            ఒకవేళ (!m_కేటాయించు_Error) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } లేకపోతే {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_కేటాయించు_Error = నిజం;
            // Likely will cause downstream errors
            తిరిగి nodep->clone_వృక్షం(అబద్ధం, need_శుద్ధ);
        }
        ఒకవేళ (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            ఒకవేళ (!m_కేటాయించు_Error) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_కేటాయించు_Error = నిజం;
            elements = 1;
            elemIdx = 0;
        }
        Ast_కణుపు_సమాసం* newp;
        ఒకవేళ (AstConsArray* స్థిరరాశి consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            స్థిరరాశి uint64_t సూచీ
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(సూచీ)->clone_వృక్షం(అబద్ధం, need_శుద్ధ);
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_Cvt_Unpack_గమ్యం_Unpack* స్థిరరాశి cvtp = VN_CAST(nodep, Cvt_Unpack_గమ్యం_Unpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_కణుపు_సమాసం* స్థిరరాశి elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, need_శుద్ధ);
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = arrayp->subDTypep();
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            ఒకవేళ (!subArrayp || !subArrayp->is_తిరగేయి_యొక్క(elemp->dtypep())) తిరిగి elemp;
            తిరిగి కొత్త Ast_Cvt_Unpack_గమ్యం_Unpack{nodep->fileline(), elemp, subDTypep};
        } లేకపోతే ఒకవేళ (AstCond* స్థిరరాశి snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            తిరిగి కొత్త AstCond{snodep->fileline(), snodep->condp()->clone_వృక్షం(అబద్ధం, need_శుద్ధ),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, need_శుద్ధ),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, need_శుద్ధ)};
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_ముక్క_Sel* స్థిరరాశి snodep = VN_CAST(nodep, ముక్క_Sel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            స్థిరరాశి పూర్ణాంకం le_స్థానభ్రంశం = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = కొత్త AstArraySel{nodep->fileline(), snodep->fromp()->clone_వృక్షం(అబద్ధం, need_శుద్ధ),
                                   le_స్థానభ్రంశం};
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstSampled* స్థిరరాశి snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_AS(snodep->exprp(), కణుపు_సమాసం);
            Ast_కణుపు_సమాసం* స్థిరరాశి selp = cloneAndSel(exprp, elements, elemIdx, need_శుద్ధ);
            తిరిగి కొత్త AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->అంతర్గత_2()};
        } లేకపోతే ఒకవేళ (Ast_సమాసం_Stmt* స్థిరరాశి snodep = VN_CAST(nodep, సమాసం_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి ఫలితం_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, need_శుద్ధ);
            ఒకవేళ (snodep->stmtsp()) {
                తిరిగి కొత్త Ast_సమాసం_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి(),
                                       ఫలితం_Selp};
            } లేకపోతే {
                తిరిగి ఫలితం_Selp;
            }
        } లేకపోతే ఒకవేళ (VN_IS(nodep, కణుపు_Var_Ref) || VN_IS(nodep, కణుపు_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            స్థిరరాశి పూర్ణాంకం le_స్థానభ్రంశం = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = కొత్త AstArraySel{nodep->fileline(), nodep->clone_వృక్షం(అబద్ధం, need_శుద్ధ), le_స్థానభ్రంశం};
        } లేకపోతే {
            ఒకవేళ (!m_కేటాయించు_Error) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_కేటాయించు_Error = నిజం;
            // Likely will cause downstream errors
            newp = nodep->clone_వృక్షం(అబద్ధం, need_శుద్ధ);
        }
        తిరిగి newp;
    }

    తార్కిక కేటాయించు_Optimize(Ast_కణుపు_కేటాయించు* nodep) {
        // Return true if did optimization
        Ast_కణుపు_DType* స్థిరరాశి dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_కణుపు* stp = nodep->rhsp();
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtp, UnpackArrayDType);
        ఒకవేళ (!arrayp) తిరిగి అబద్ధం;
        ఒకవేళ (VN_IS(stp, Cvt_Packed_గమ్యం_Array)) తిరిగి అబద్ధం;
        ఒకవేళ (VN_IS(stp, CReset)) తిరిగి అబద్ధం;

        // Any isSc variables must be expanded regardless of --fno-slice
        స్థిరరాశి తార్కిక hasSc
            = nodep->exists([&](స్థిరరాశి AstVarRef* refp) -> తార్కిక { తిరిగి refp->varp()->isSc(); });
        ఒకవేళ (!hasSc && !v3Global.opt.f_ముక్క()) {
            m_okConsArray = నిజం;  // VL_RESTORER in visit(AstNodeAssign)
            తిరిగి అబద్ధం;
        }

        // Skip optimization if array is too large
        స్థిరరాశి పూర్ణాంకం elements = arrayp->rangep()->elementsConst();
        స్థిరరాశి పూర్ణాంకం element_హద్దు = v3Global.opt.f_ముక్క_Element_హద్దు();
        ఒకవేళ (elements > element_హద్దు && element_హద్దు > 0) {
            ++m_stat_ముక్క_Element_Skips;
            m_okConsArray = నిజం;  // VL_RESTORER in visit(AstNodeAssign)
            తిరిగి అబద్ధం;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        ఒకవేళ (AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ఒకవేళ (AstVarRef* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ఒకవేళ (!hasSc && lhsp->dtypep()->skipRefp()->same_వృక్షం(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = నిజం;  // VL_RESTORER in visit(AstNodeAssign)
                    తిరిగి అబద్ధం;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_కణుపు_కేటాయించు* newlistp = శూన్యసూచిక;
        కొరకు (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_కణుపు_కేటాయించు* స్థిరరాశి newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_కణుపు::కూడు_తదుపరి(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // Called recursively on newly created assignments
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_కేటాయించు_Error);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_కేటాయించు_Error = అబద్ధం;
        m_assignp = nodep;
        ఒకవేళ (కేటాయించు_Optimize(nodep)) తిరిగి;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    మూస <రకం_పేరు T_కణుపు_Biop>
    శూన్య expandBiOp(T_కణుపు_Biop* biopp) {
        Ast_కణుపు_Biop* nodep = biopp;
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_కణుపు_Biop* logp = శూన్యసూచిక;

            స్థిరరాశి పూర్ణాంకం elements = adtypep->rangep()->elementsConst();
            కొరకు (పూర్ణాంకం elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_కణుపు_Biop* స్థిరరాశి clonep = కొత్త T_కణుపు_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                ఒకవేళ (!logp) {
                    logp = clonep;
                } లేకపోతే {
                    ఎంపిక (nodep->type()) {
                    స్థితి VNType::Eq:  // FALLTHRU
                    స్థితి VNType::EqCase:
                        logp = కొత్త AstLogAnd{nodep->fileline(), logp, clonep};
                        ఆపు;
                    స్థితి VNType::Neq:  // FALLTHRU
                    స్థితి VNType::NeqCase:
                        logp = కొత్త AstLogOr{nodep->fileline(), logp, clonep};
                        ఆపు;
                    అప్రమేయ: nodep->v3fatalSrc("Unknown node type processing array slice"); ఆపు;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstEq* nodep) override { expandBiOp(nodep); }
    శూన్య సందర్శించు(AstNeq* nodep) override { expandBiOp(nodep); }
    శూన్య సందర్శించు(AstEqCase* nodep) override { expandBiOp(nodep); }
    శూన్య సందర్శించు(AstNeqCase* nodep) override { expandBiOp(nodep); }

    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp()->unlink_Fr_వెనుక();
        AstConst* స్థిరరాశి lop = కొత్త AstConst{nodep->fileline(), AstConst::Widthed_విలువ{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* స్థిరరాశి newp
            = కొత్త AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_ముక్క};
        newp->కూడు_Pinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->did_వెడల్పు(నిజం);
        newp->protect(అబద్ధం);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ముక్క_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ముక్క_Visitor() override {
        V3Stats::కూడు_Stat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::కూడు_Stat("Optimizations, Slice array skips due to size limit",
                         m_stat_ముక్క_Element_Skips);
    }
};

//######################################################################
// Link class functions

శూన్య V3_ముక్క::ముక్క_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ముక్క_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("slice", 0, dump_వృక్షం_Either_Level() >= 3);
}
