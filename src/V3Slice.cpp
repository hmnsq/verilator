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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Slice.h"

#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

ವರ್ಗ SliceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು* m_assignp = nullptr;  // Assignment we are under
    ತಾರ್ಕಿಕ m_assignError = ಅಸತ್ಯ;  // True if the current assign already has an error
    ತಾರ್ಕಿಕ m_okConsArray = ಅಸತ್ಯ;  // Allow ConsArray children

    // METHODS
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cloneAndSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep, uint64_t elements, uint64_t elemIdx,
                             ಧ್ರುವ ತಾರ್ಕಿಕ needPure) {
        // Insert an ArraySel, except for a few special cases
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಯದಿ (!arrayp) {  // V3Width should have complained, but...
            ಯದಿ (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } ಅನ್ಯಥಾ {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = ಸತ್ಯ;
            // Likely will cause downstream errors
            ಪ್ರತ್ಯಾಗಮ nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ, needPure);
        }
        ಯದಿ (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            ಯದಿ (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = ಸತ್ಯ;
            elements = 1;
            elemIdx = 0;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
        ಯದಿ (AstConsArray* ಧ್ರುವ consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ಧ್ರುವ uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ, needPure);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCvtUnpackToUnpack* ಧ್ರುವ cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = arrayp->subDTypep();
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            ಯದಿ (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) ಪ್ರತ್ಯಾಗಮ elemp;
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } ಅನ್ಯಥಾ ಯದಿ (AstCond* ಧ್ರುವ snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{snodep->fileline(), snodep->condp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSliceSel* ಧ್ರುವ snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = ನೂತನ AstArraySel{nodep->fileline(), snodep->fromp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ, needPure),
                                   leOffset};
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstSampled* ಧ್ರುವ snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_AS(snodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ snodep = VN_CAST(nodep, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            ಯದಿ (snodep->stmtsp()) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ resultSelp;
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, ಗಂಟು_Var_Ref) || VN_IS(nodep, ಗಂಟು_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = ನೂತನ AstArraySel{nodep->fileline(), nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ, needPure), leOffset};
        } ಅನ್ಯಥಾ {
            ಯದಿ (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = ಸತ್ಯ;
            // Likely will cause downstream errors
            newp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ, needPure);
        }
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    ತಾರ್ಕಿಕ assignOptimize(Ast_ಗಂಟು_Assign* nodep) {
        // Return true if did optimization
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_ಗಂಟು* stp = nodep->rhsp();
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtp, UnpackArrayDType);
        ಯದಿ (!arrayp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_IS(stp, CvtPackedToArray)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_IS(stp, CReset)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Any isSc variables must be expanded regardless of --fno-slice
        ಧ್ರುವ ತಾರ್ಕಿಕ hasSc
            = nodep->exists([&](ಧ್ರುವ AstVarRef* refp) -> ತಾರ್ಕಿಕ { ಪ್ರತ್ಯಾಗಮ refp->varp()->isSc(); });
        ಯದಿ (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = ಸತ್ಯ;  // VL_RESTORER in visit(AstNodeAssign)
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Skip optimization if array is too large
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements = arrayp->rangep()->elementsConst();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elementLimit = v3Global.opt.fSliceElementLimit();
        ಯದಿ (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = ಸತ್ಯ;  // VL_RESTORER in visit(AstNodeAssign)
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        ಯದಿ (AstVarRef* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ಯದಿ (AstVarRef* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ಯದಿ (!hasSc && lhsp->dtypep()->skipRefp()->same_ವೃಕ್ಷ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = ಸತ್ಯ;  // VL_RESTORER in visit(AstNodeAssign)
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_ಗಂಟು_Assign* newlistp = nullptr;
        ಪ್ರತಿ (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_ಗಂಟು_Assign* ಧ್ರುವ newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_ಗಂಟು::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // Called recursively on newly created assignments
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = ಅಸತ್ಯ;
        m_assignp = nodep;
        ಯದಿ (assignOptimize(nodep)) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_ಗಂಟು_Biop>
    ಶೂನ್ಯ expandBiOp(T_ಗಂಟು_Biop* biopp) {
        Ast_ಗಂಟು_Biop* nodep = biopp;
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_ಗಂಟು_Biop* logp = nullptr;

            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements = adtypep->rangep()->elementsConst();
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_ಗಂಟು_Biop* ಧ್ರುವ clonep = ನೂತನ T_ಗಂಟು_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                ಯದಿ (!logp) {
                    logp = clonep;
                } ಅನ್ಯಥಾ {
                    ವಿಕಲ್ಪ (nodep->type()) {
                    ಪಕ್ಷ VNType::Eq:  // FALLTHRU
                    ಪಕ್ಷ VNType::EqCase:
                        logp = ನೂತನ AstLogAnd{nodep->fileline(), logp, clonep};
                        ಭಙ್ಗ;
                    ಪಕ್ಷ VNType::Neq:  // FALLTHRU
                    ಪಕ್ಷ VNType::NeqCase:
                        logp = ನೂತನ AstLogOr{nodep->fileline(), logp, clonep};
                        ಭಙ್ಗ;
                    ಸ್ವಭಾವ: nodep->v3fatalSrc("Unknown node type processing array slice"); ಭಙ್ಗ;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstEq* nodep) override { expandBiOp(nodep); }
    ಶೂನ್ಯ visit(AstNeq* nodep) override { expandBiOp(nodep); }
    ಶೂನ್ಯ visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    ಶೂನ್ಯ visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    ಶೂನ್ಯ visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ಧ್ರುವ lop = ನೂತನ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ಧ್ರುವ newp
            = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(ಸತ್ಯ);
        newp->protect(ಅಸತ್ಯ);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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

ಶೂನ್ಯ V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("slice", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
