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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Slice.h"

#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

ଶ୍ରେଣୀ SliceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି* m_assignp = nullptr;  // Assignment we are under
    ତାର୍କିକ m_assignError = ମିଥ୍ୟା;  // True if the current assign already has an error
    ତାର୍କିକ m_okConsArray = ମିଥ୍ୟା;  // Allow ConsArray children

    // METHODS
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cloneAndSel(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep, uint64_t elements, uint64_t elemIdx,
                             ଅଚଳ ତାର୍କିକ needPure) {
        // Insert an ArraySel, except for a few special cases
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ଯଦି (!arrayp) {  // V3Width should have complained, but...
            ଯଦି (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } ଅନ୍ୟଥା {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = ସତ୍ୟ;
            // Likely will cause downstream errors
            ଫେରନ୍ତୁ nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା, needPure);
        }
        ଯଦି (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            ଯଦି (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = ସତ୍ୟ;
            elements = 1;
            elemIdx = 0;
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
        ଯଦି (AstConsArray* ଅଚଳ consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ଅଚଳ uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା, needPure);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCvtUnpackToUnpack* ଅଚଳ cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subDTypep = arrayp->subDTypep();
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            ଯଦି (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) ଫେରନ୍ତୁ elemp;
            ଫେରନ୍ତୁ ନୂତନ AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } ଅନ୍ୟଥା ଯଦି (AstCond* ଅଚଳ snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ଫେରନ୍ତୁ ନୂତନ AstCond{snodep->fileline(), snodep->condp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSliceSel* ଅଚଳ snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = ନୂତନ AstArraySel{nodep->fileline(), snodep->fromp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା, needPure),
                                   leOffset};
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstSampled* ଅଚଳ snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_AS(snodep->exprp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            ଫେରନ୍ତୁ ନୂତନ AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } ଅନ୍ୟଥା ଯଦି (Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ snodep = VN_CAST(nodep, ପଦସମଷ୍ଟି_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            ଯଦି (snodep->stmtsp()) {
                ଫେରନ୍ତୁ ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ resultSelp;
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_Var_Ref) || VN_IS(nodep, ଗଣ୍ଠି_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = ନୂତନ AstArraySel{nodep->fileline(), nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା, needPure), leOffset};
        } ଅନ୍ୟଥା {
            ଯଦି (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = ସତ୍ୟ;
            // Likely will cause downstream errors
            newp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା, needPure);
        }
        ଫେରନ୍ତୁ newp;
    }

    ତାର୍କିକ assignOptimize(Ast_ଗଣ୍ଠି_Assign* nodep) {
        // Return true if did optimization
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_ଗଣ୍ଠି* stp = nodep->rhsp();
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrayp = VN_CAST(dtp, UnpackArrayDType);
        ଯଦି (!arrayp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_IS(stp, CvtPackedToArray)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_IS(stp, CReset)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Any isSc variables must be expanded regardless of --fno-slice
        ଅଚଳ ତାର୍କିକ hasSc
            = nodep->exists([&](ଅଚଳ AstVarRef* refp) -> ତାର୍କିକ { ଫେରନ୍ତୁ refp->varp()->isSc(); });
        ଯଦି (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = ସତ୍ୟ;  // VL_RESTORER in visit(AstNodeAssign)
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Skip optimization if array is too large
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = arrayp->rangep()->elementsConst();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elementLimit = v3Global.opt.fSliceElementLimit();
        ଯଦି (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = ସତ୍ୟ;  // VL_RESTORER in visit(AstNodeAssign)
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        ଯଦି (AstVarRef* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ଯଦି (AstVarRef* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ଯଦି (!hasSc && lhsp->dtypep()->skipRefp()->same_ବୃକ୍ଷ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = ସତ୍ୟ;  // VL_RESTORER in visit(AstNodeAssign)
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_ଗଣ୍ଠି_Assign* newlistp = nullptr;
        ପାଇଁ (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_ଗଣ୍ଠି_Assign* ଅଚଳ newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_ଗଣ୍ଠି::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        // Called recursively on newly created assignments
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = ମିଥ୍ୟା;
        m_assignp = nodep;
        ଯଦି (assignOptimize(nodep)) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_ଗଣ୍ଠି_Biop>
    ଶୂନ୍ୟ expandBiOp(T_ଗଣ୍ଠି_Biop* biopp) {
        Ast_ଗଣ୍ଠି_Biop* nodep = biopp;
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_ଗଣ୍ଠି_Biop* logp = nullptr;

            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements = adtypep->rangep()->elementsConst();
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_ଗଣ୍ଠି_Biop* ଅଚଳ clonep = ନୂତନ T_ଗଣ୍ଠି_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                ଯଦି (!logp) {
                    logp = clonep;
                } ଅନ୍ୟଥା {
                    ବାଛ (nodep->type()) {
                    ସ୍ଥିତି VNType::Eq:  // FALLTHRU
                    ସ୍ଥିତି VNType::EqCase:
                        logp = ନୂତନ AstLogAnd{nodep->fileline(), logp, clonep};
                        ଭାଙ୍ଗ;
                    ସ୍ଥିତି VNType::Neq:  // FALLTHRU
                    ସ୍ଥିତି VNType::NeqCase:
                        logp = ନୂତନ AstLogOr{nodep->fileline(), logp, clonep};
                        ଭାଙ୍ଗ;
                    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Unknown node type processing array slice"); ଭାଙ୍ଗ;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstEq* nodep) override { expandBiOp(nodep); }
    ଶୂନ୍ୟ visit(AstNeq* nodep) override { expandBiOp(nodep); }
    ଶୂନ୍ୟ visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    ଶୂନ୍ୟ visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ଅଚଳ lop = ନୂତନ AstConst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ଅଚଳ newp
            = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(ସତ୍ୟ);
        newp->protect(ମିଥ୍ୟା);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
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

ଶୂନ୍ୟ V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("slice", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
