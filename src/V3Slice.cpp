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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Slice.h"

#অন্তর্ভাৱ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

শ্ৰেণী SliceVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ধ্রুৱ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্ৰন্থি* m_assignp = nullptr;  // Assignment we are under
    তার্কিক m_assignError = মিছা;  // True if the current assign already has an error
    তার্কিক m_okConsArray = মিছা;  // Allow ConsArray children

    // METHODS
    Ast_গ্ৰন্থি_ৰাশিমালা* cloneAndSel(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nodep, uint64_t elements, uint64_t elemIdx,
                             ধ্রুৱ তার্কিক needPure) {
        // Insert an ArraySel, except for a few special cases
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (!arrayp) {  // V3Width should have complained, but...
            যদি (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } অন্যথা {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = সঁচা;
            // Likely will cause downstream errors
            ঘূৰাই nodep->clone_বৃক্ষ(মিছা, needPure);
        }
        যদি (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            যদি (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = সঁচা;
            elements = 1;
            elemIdx = 0;
        }
        Ast_গ্ৰন্থি_ৰাশিমালা* newp;
        যদি (AstConsArray* ধ্রুৱ consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ধ্রুৱ uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_বৃক্ষ(মিছা, needPure);
        } অন্যথা যদি (ধ্রুৱ AstCvtUnpackToUnpack* ধ্রুৱ cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = arrayp->subDTypep();
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            যদি (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) ঘূৰাই elemp;
            ঘূৰাই নূতন AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } অন্যথা যদি (AstCond* ধ্রুৱ snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ঘূৰাই নূতন AstCond{snodep->fileline(), snodep->condp()->clone_বৃক্ষ(মিছা, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } অন্যথা যদি (ধ্রুৱ AstSliceSel* ধ্রুৱ snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ধ্রুৱ পূর্ণাঙ্ক leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = নূতন AstArraySel{nodep->fileline(), snodep->fromp()->clone_বৃক্ষ(মিছা, needPure),
                                   leOffset};
        } অন্যথা যদি (ধ্রুৱ AstSampled* ধ্রুৱ snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = VN_AS(snodep->exprp(), গ্ৰন্থি_ৰাশিমালা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            ঘূৰাই নূতন AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } অন্যথা যদি (Ast_ৰাশিমালা_Stmt* ধ্রুৱ snodep = VN_CAST(nodep, ৰাশিমালা_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            যদি (snodep->stmtsp()) {
                ঘূৰাই নূতন Ast_ৰাশিমালা_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } অন্যথা {
                ঘূৰাই resultSelp;
            }
        } অন্যথা যদি (VN_IS(nodep, গ্ৰন্থি_Var_Ref) || VN_IS(nodep, গ্ৰন্থি_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ধ্রুৱ পূর্ণাঙ্ক leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = নূতন AstArraySel{nodep->fileline(), nodep->clone_বৃক্ষ(মিছা, needPure), leOffset};
        } অন্যথা {
            যদি (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = সঁচা;
            // Likely will cause downstream errors
            newp = nodep->clone_বৃক্ষ(মিছা, needPure);
        }
        ঘূৰাই newp;
    }

    তার্কিক assignOptimize(Ast_গ্ৰন্থি_Assign* nodep) {
        // Return true if did optimization
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_গ্ৰন্থি* stp = nodep->rhsp();
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrayp = VN_CAST(dtp, UnpackArrayDType);
        যদি (!arrayp) ঘূৰাই মিছা;
        যদি (VN_IS(stp, CvtPackedToArray)) ঘূৰাই মিছা;
        যদি (VN_IS(stp, CReset)) ঘূৰাই মিছা;

        // Any isSc variables must be expanded regardless of --fno-slice
        ধ্রুৱ তার্কিক hasSc
            = nodep->exists([&](ধ্রুৱ AstVarRef* refp) -> তার্কিক { ঘূৰাই refp->varp()->isSc(); });
        যদি (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = সঁচা;  // VL_RESTORER in visit(AstNodeAssign)
            ঘূৰাই মিছা;
        }

        // Skip optimization if array is too large
        ধ্রুৱ পূর্ণাঙ্ক elements = arrayp->rangep()->elementsConst();
        ধ্রুৱ পূর্ণাঙ্ক elementLimit = v3Global.opt.fSliceElementLimit();
        যদি (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = সঁচা;  // VL_RESTORER in visit(AstNodeAssign)
            ঘূৰাই মিছা;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        যদি (AstVarRef* ধ্রুৱ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            যদি (AstVarRef* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                যদি (!hasSc && lhsp->dtypep()->skipRefp()->same_বৃক্ষ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = সঁচা;  // VL_RESTORER in visit(AstNodeAssign)
                    ঘূৰাই মিছা;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_গ্ৰন্থি_Assign* newlistp = nullptr;
        বাবে (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_গ্ৰন্থি_Assign* ধ্রুৱ newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_গ্ৰন্থি::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ঘূৰাই সঁচা;
    }

    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        // Called recursively on newly created assignments
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = মিছা;
        m_assignp = nodep;
        যদি (assignOptimize(nodep)) ঘূৰাই;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = সঁচা;
        iterateChildren(nodep);
    }
    শূন্য visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_গ্ৰন্থি_Biop>
    শূন্য expandBiOp(T_গ্ৰন্থি_Biop* biopp) {
        Ast_গ্ৰন্থি_Biop* nodep = biopp;
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_গ্ৰন্থি_Biop* logp = nullptr;

            ধ্রুৱ পূর্ণাঙ্ক elements = adtypep->rangep()->elementsConst();
            বাবে (পূর্ণাঙ্ক elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_গ্ৰন্থি_Biop* ধ্রুৱ clonep = নূতন T_গ্ৰন্থি_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                যদি (!logp) {
                    logp = clonep;
                } অন্যথা {
                    ৱিকল্প (nodep->type()) {
                    পক্ষ VNType::Eq:  // FALLTHRU
                    পক্ষ VNType::EqCase:
                        logp = নূতন AstLogAnd{nodep->fileline(), logp, clonep};
                        ভঙ্গ;
                    পক্ষ VNType::Neq:  // FALLTHRU
                    পক্ষ VNType::NeqCase:
                        logp = নূতন AstLogOr{nodep->fileline(), logp, clonep};
                        ভঙ্গ;
                    স্ৱভাৱ: nodep->v3fatalSrc("Unknown node type processing array slice"); ভঙ্গ;
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
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ধ্রুৱ lop = নূতন AstConst{nodep->fileline(), AstConst::Widthed_মান{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ধ্রুৱ newp
            = নূতন AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(সঁচা);
        newp->protect(মিছা);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
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
