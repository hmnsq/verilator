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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Slice.h"

#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

শ্রেণী SliceVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ধ্রুবক VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্রন্থি* m_assignp = nullptr;  // Assignment we are under
    যৌক্তিক m_assignError = মিথ্যা;  // True if the current assign already has an error
    যৌক্তিক m_okConsArray = মিথ্যা;  // Allow ConsArray children

    // METHODS
    Ast_গ্রন্থি_রাশিমালা* cloneAndSel(Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nodep, uint64_t elements, uint64_t elemIdx,
                             ধ্রুবক যৌক্তিক needPure) {
        // Insert an ArraySel, except for a few special cases
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (!arrayp) {  // V3Width should have complained, but...
            যদি (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } অন্যথা {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = সত্য;
            // Likely will cause downstream errors
            ফেরত nodep->clone_বৃক্ষ(মিথ্যা, needPure);
        }
        যদি (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            যদি (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = সত্য;
            elements = 1;
            elemIdx = 0;
        }
        Ast_গ্রন্থি_রাশিমালা* newp;
        যদি (AstConsArray* ধ্রুবক consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ধ্রুবক uint64_t সূচক
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(সূচক)->clone_বৃক্ষ(মিথ্যা, needPure);
        } অন্যথা যদি (ধ্রুবক AstCvtUnpackToUnpack* ধ্রুবক cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_গ্রন্থি_DType* ধ্রুবক subDTypep = arrayp->subDTypep();
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            যদি (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) ফেরত elemp;
            ফেরত নতুন AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } অন্যথা যদি (AstCond* ধ্রুবক snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ফেরত নতুন AstCond{snodep->fileline(), snodep->condp()->clone_বৃক্ষ(মিথ্যা, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } অন্যথা যদি (ধ্রুবক AstSliceSel* ধ্রুবক snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ধ্রুবক পূর্ণসংখ্যা leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = নতুন AstArraySel{nodep->fileline(), snodep->fromp()->clone_বৃক্ষ(মিথ্যা, needPure),
                                   leOffset};
        } অন্যথা যদি (ধ্রুবক AstSampled* ধ্রুবক snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_AS(snodep->exprp(), গ্রন্থি_রাশিমালা);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            ফেরত নতুন AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } অন্যথা যদি (Ast_রাশিমালা_Stmt* ধ্রুবক snodep = VN_CAST(nodep, রাশিমালা_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক ফলাফল_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            যদি (snodep->stmtsp()) {
                ফেরত নতুন Ast_রাশিমালা_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী(),
                                       ফলাফল_Selp};
            } অন্যথা {
                ফেরত ফলাফল_Selp;
            }
        } অন্যথা যদি (VN_IS(nodep, গ্রন্থি_Var_Ref) || VN_IS(nodep, গ্রন্থি_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ধ্রুবক পূর্ণসংখ্যা leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = নতুন AstArraySel{nodep->fileline(), nodep->clone_বৃক্ষ(মিথ্যা, needPure), leOffset};
        } অন্যথা {
            যদি (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = সত্য;
            // Likely will cause downstream errors
            newp = nodep->clone_বৃক্ষ(মিথ্যা, needPure);
        }
        ফেরত newp;
    }

    যৌক্তিক assignOptimize(Ast_গ্রন্থি_Assign* nodep) {
        // Return true if did optimization
        Ast_গ্রন্থি_DType* ধ্রুবক dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_গ্রন্থি* stp = nodep->rhsp();
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক arrayp = VN_CAST(dtp, UnpackArrayDType);
        যদি (!arrayp) ফেরত মিথ্যা;
        যদি (VN_IS(stp, CvtPackedToArray)) ফেরত মিথ্যা;
        যদি (VN_IS(stp, CReset)) ফেরত মিথ্যা;

        // Any isSc variables must be expanded regardless of --fno-slice
        ধ্রুবক যৌক্তিক hasSc
            = nodep->exists([&](ধ্রুবক AstVarRef* refp) -> যৌক্তিক { ফেরত refp->varp()->isSc(); });
        যদি (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = সত্য;  // VL_RESTORER in visit(AstNodeAssign)
            ফেরত মিথ্যা;
        }

        // Skip optimization if array is too large
        ধ্রুবক পূর্ণসংখ্যা elements = arrayp->rangep()->elementsConst();
        ধ্রুবক পূর্ণসংখ্যা elementLimit = v3Global.opt.fSliceElementLimit();
        যদি (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = সত্য;  // VL_RESTORER in visit(AstNodeAssign)
            ফেরত মিথ্যা;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        যদি (AstVarRef* ধ্রুবক lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            যদি (AstVarRef* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                যদি (!hasSc && lhsp->dtypep()->skipRefp()->same_বৃক্ষ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = সত্য;  // VL_RESTORER in visit(AstNodeAssign)
                    ফেরত মিথ্যা;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_গ্রন্থি_Assign* newlistp = nullptr;
        জন্য (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_গ্রন্থি_Assign* ধ্রুবক newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_গ্রন্থি::add_পরবর্তী(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ফেরত সত্য;
    }

    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // Called recursively on newly created assignments
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = মিথ্যা;
        m_assignp = nodep;
        যদি (assignOptimize(nodep)) ফেরত;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_গ্রন্থি_Biop>
    শূন্য expandBiOp(T_গ্রন্থি_Biop* biopp) {
        Ast_গ্রন্থি_Biop* nodep = biopp;
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_গ্রন্থি_Biop* logp = nullptr;

            ধ্রুবক পূর্ণসংখ্যা elements = adtypep->rangep()->elementsConst();
            জন্য (পূর্ণসংখ্যা elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_গ্রন্থি_Biop* ধ্রুবক clonep = নতুন T_গ্রন্থি_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                যদি (!logp) {
                    logp = clonep;
                } অন্যথা {
                    নির্বাচন (nodep->type()) {
                    ক্ষেত্র VNType::Eq:  // FALLTHRU
                    ক্ষেত্র VNType::EqCase:
                        logp = নতুন AstLogAnd{nodep->fileline(), logp, clonep};
                        বিরতি;
                    ক্ষেত্র VNType::Neq:  // FALLTHRU
                    ক্ষেত্র VNType::NeqCase:
                        logp = নতুন AstLogOr{nodep->fileline(), logp, clonep};
                        বিরতি;
                    পূর্বনির্ধারিত: nodep->v3fatalSrc("Unknown node type processing array slice"); বিরতি;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    শূন্য visit(AstEq* nodep) override { expandBiOp(nodep); }
    শূন্য visit(AstNeq* nodep) override { expandBiOp(nodep); }
    শূন্য visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    শূন্য visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    শূন্য visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ধ্রুবক lop = নতুন AstConst{nodep->fileline(), AstConst::Widthed_মান{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ধ্রুবক newp
            = নতুন AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(সত্য);
        newp->protect(মিথ্যা);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit SliceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SliceVisitor() override {
        V3Stats::addStat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::addStat("Optimizations, Slice array skips due to size limit",
                         m_statSliceElementSkips);
    }
};

//######################################################################
// Link class functions

শূন্য V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("slice", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
