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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Slice.h"

#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

صنف SliceVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ثابت VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstNode* m_assignp = nullptr;  // Assignment we are under
    منطقي m_assignError = ܕܓܠܐ;  // True if the current assign already has an error
    منطقي m_okConsArray = ܕܓܠܐ;  // Allow ConsArray children

    // METHODS
    AstNodeExpr* cloneAndSel(AstNodeExpr* ثابت nodep, uint64_t elements, uint64_t elemIdx,
                             ثابت منطقي needPure) {
        // Insert an ArraySel, except for a few special cases
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ܐܢ (!arrayp) {  // V3Width should have complained, but...
            ܐܢ (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } ܐܚܪܢܐ {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = ܫܪܪܐ;
            // Likely will cause downstream errors
            ܦܢܐ nodep->cloneTree(ܕܓܠܐ, needPure);
        }
        ܐܢ (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            ܐܢ (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = ܫܪܪܐ;
            elements = 1;
            elemIdx = 0;
        }
        AstNodeExpr* newp;
        ܐܢ (AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ثابت uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->cloneTree(ܕܓܠܐ, needPure);
        } ܐܚܪܢܐ ܐܢ (ثابت AstCvtUnpackToUnpack* ثابت cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            AstNodeExpr* ثابت elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            AstNodeDType* ثابت subDTypep = arrayp->subDTypep();
            ثابت AstUnpackArrayDType* ثابت subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            ܐܢ (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) ܦܢܐ elemp;
            ܦܢܐ جديد AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } ܐܚܪܢܐ ܐܢ (AstCond* ثابت snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ܦܢܐ جديد AstCond{snodep->fileline(), snodep->condp()->cloneTree(ܕܓܠܐ, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } ܐܚܪܢܐ ܐܢ (ثابت AstSliceSel* ثابت snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحيح leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = جديد AstArraySel{nodep->fileline(), snodep->fromp()->cloneTree(ܕܓܠܐ, needPure),
                                   leOffset};
        } ܐܚܪܢܐ ܐܢ (ثابت AstSampled* ثابت snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            AstNodeExpr* ثابت exprp = VN_AS(snodep->exprp(), NodeExpr);
            AstNodeExpr* ثابت selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            ܦܢܐ جديد AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } ܐܚܪܢܐ ܐܢ (AstExprStmt* ثابت snodep = VN_CAST(nodep, ExprStmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            AstNodeExpr* ثابت resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            ܐܢ (snodep->stmtsp()) {
                ܦܢܐ جديد AstExprStmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } ܐܚܪܢܐ {
                ܦܢܐ resultSelp;
            }
        } ܐܚܪܢܐ ܐܢ (VN_IS(nodep, NodeVarRef) || VN_IS(nodep, NodeSel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحيح leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = جديد AstArraySel{nodep->fileline(), nodep->cloneTree(ܕܓܠܐ, needPure), leOffset};
        } ܐܚܪܢܐ {
            ܐܢ (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = ܫܪܪܐ;
            // Likely will cause downstream errors
            newp = nodep->cloneTree(ܕܓܠܐ, needPure);
        }
        ܦܢܐ newp;
    }

    منطقي assignOptimize(AstNodeAssign* nodep) {
        // Return true if did optimization
        AstNodeDType* ثابت dtp = nodep->lhsp()->dtypep()->skipRefp();
        AstNode* stp = nodep->rhsp();
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtp, UnpackArrayDType);
        ܐܢ (!arrayp) ܦܢܐ ܕܓܠܐ;
        ܐܢ (VN_IS(stp, CvtPackedToArray)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (VN_IS(stp, CReset)) ܦܢܐ ܕܓܠܐ;

        // Any isSc variables must be expanded regardless of --fno-slice
        ثابت منطقي hasSc
            = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي { ܦܢܐ refp->varp()->isSc(); });
        ܐܢ (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = ܫܪܪܐ;  // VL_RESTORER in visit(AstNodeAssign)
            ܦܢܐ ܕܓܠܐ;
        }

        // Skip optimization if array is too large
        ثابت صحيح elements = arrayp->rangep()->elementsConst();
        ثابت صحيح elementLimit = v3Global.opt.fSliceElementLimit();
        ܐܢ (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = ܫܪܪܐ;  // VL_RESTORER in visit(AstNodeAssign)
            ܦܢܐ ܕܓܠܐ;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        ܐܢ (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ܐܢ (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ܐܢ (!hasSc && lhsp->dtypep()->skipRefp()->sameTree(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = ܫܪܪܐ;  // VL_RESTORER in visit(AstNodeAssign)
                    ܦܢܐ ܕܓܠܐ;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        AstNodeAssign* newlistp = nullptr;
        لكل (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            AstNodeAssign* ثابت newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = AstNode::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ܦܢܐ ܫܪܪܐ;
    }

    فراغ visit(AstNodeAssign* nodep) override {
        // Called recursively on newly created assignments
        ܐܢ (nodep->user1SetOnce()) ܦܢܐ;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = ܕܓܠܐ;
        m_assignp = nodep;
        ܐܢ (assignOptimize(nodep)) ܦܢܐ;
        iterateChildren(nodep);
    }

    فراغ visit(AstNodeCons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = ܫܪܪܐ;
        iterateChildren(nodep);
    }
    فراغ visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_NodeBiop>
    فراغ expandBiOp(T_NodeBiop* biopp) {
        AstNodeBiop* nodep = biopp;
        ܐܢ (nodep->user1SetOnce()) ܦܢܐ;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ثابت AstNodeDType* ثابت fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        ܐܢ (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            AstNodeBiop* logp = nullptr;

            ثابت صحيح elements = adtypep->rangep()->elementsConst();
            لكل (صحيح elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_NodeBiop* ثابت clonep = جديد T_NodeBiop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                ܐܢ (!logp) {
                    logp = clonep;
                } ܐܚܪܢܐ {
                    بدل (nodep->type()) {
                    حالة VNType::Eq:  // FALLTHRU
                    حالة VNType::EqCase:
                        logp = جديد AstLogAnd{nodep->fileline(), logp, clonep};
                        اكسر;
                    حالة VNType::Neq:  // FALLTHRU
                    حالة VNType::NeqCase:
                        logp = جديد AstLogOr{nodep->fileline(), logp, clonep};
                        اكسر;
                    افتراضي: nodep->v3fatalSrc("Unknown node type processing array slice"); اكسر;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    فراغ visit(AstEq* nodep) override { expandBiOp(nodep); }
    فراغ visit(AstNeq* nodep) override { expandBiOp(nodep); }
    فراغ visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    فراغ visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    فراغ visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        AstNodeExpr* ثابت fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ثابت lop = جديد AstConst{nodep->fileline(), AstConst::WidthedValue{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ثابت newp
            = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(ܫܪܪܐ);
        newp->protect(ܕܓܠܐ);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
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

فراغ V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("slice", 0, dumpTreeEitherLevel() >= 3);
}
