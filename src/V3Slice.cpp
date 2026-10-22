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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Slice.h"

#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

رده SliceVisitor final : عمومی VNVisitor {
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
    Ast_گره* m_assignp = nullptr;  // Assignment we are under
    منطقی m_assignError = نادرست;  // True if the current assign already has an error
    منطقی m_okConsArray = نادرست;  // Allow ConsArray children

    // METHODS
    Ast_گره_عبارت* cloneAndSel(Ast_گره_عبارت* ثابت nodep, uint64_t elements, uint64_t elemIdx,
                             ثابت منطقی needPure) {
        // Insert an ArraySel, except for a few special cases
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (!arrayp) {  // V3Width should have complained, but...
            اگر (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } وگرنه {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = درست;
            // Likely will cause downstream errors
            بازگشت nodep->clone_درخت(نادرست, needPure);
        }
        اگر (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            اگر (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = درست;
            elements = 1;
            elemIdx = 0;
        }
        Ast_گره_عبارت* newp;
        اگر (AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ثابت uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_درخت(نادرست, needPure);
        } وگرنه اگر (ثابت AstCvtUnpackToUnpack* ثابت cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_گره_عبارت* ثابت elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_گره_DType* ثابت subDTypep = arrayp->subDTypep();
            ثابت AstUnpackArrayDType* ثابت subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            اگر (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) بازگشت elemp;
            بازگشت جدید AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } وگرنه اگر (AstCond* ثابت snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            بازگشت جدید AstCond{snodep->fileline(), snodep->condp()->clone_درخت(نادرست, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } وگرنه اگر (ثابت AstSliceSel* ثابت snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحیح leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = جدید AstArraySel{nodep->fileline(), snodep->fromp()->clone_درخت(نادرست, needPure),
                                   leOffset};
        } وگرنه اگر (ثابت AstSampled* ثابت snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_گره_عبارت* ثابت exprp = VN_AS(snodep->exprp(), گره_عبارت);
            Ast_گره_عبارت* ثابت selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            بازگشت جدید AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } وگرنه اگر (Ast_عبارت_Stmt* ثابت snodep = VN_CAST(nodep, عبارت_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_گره_عبارت* ثابت resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            اگر (snodep->stmtsp()) {
                بازگشت جدید Ast_عبارت_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } وگرنه {
                بازگشت resultSelp;
            }
        } وگرنه اگر (VN_IS(nodep, گره_Var_Ref) || VN_IS(nodep, گره_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحیح leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = جدید AstArraySel{nodep->fileline(), nodep->clone_درخت(نادرست, needPure), leOffset};
        } وگرنه {
            اگر (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = درست;
            // Likely will cause downstream errors
            newp = nodep->clone_درخت(نادرست, needPure);
        }
        بازگشت newp;
    }

    منطقی assignOptimize(Ast_گره_Assign* nodep) {
        // Return true if did optimization
        Ast_گره_DType* ثابت dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_گره* stp = nodep->rhsp();
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtp, UnpackArrayDType);
        اگر (!arrayp) بازگشت نادرست;
        اگر (VN_IS(stp, CvtPackedToArray)) بازگشت نادرست;
        اگر (VN_IS(stp, CReset)) بازگشت نادرست;

        // Any isSc variables must be expanded regardless of --fno-slice
        ثابت منطقی hasSc
            = nodep->exists([&](ثابت AstVarRef* refp) -> منطقی { بازگشت refp->varp()->isSc(); });
        اگر (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = درست;  // VL_RESTORER in visit(AstNodeAssign)
            بازگشت نادرست;
        }

        // Skip optimization if array is too large
        ثابت صحیح elements = arrayp->rangep()->elementsConst();
        ثابت صحیح elementLimit = v3Global.opt.fSliceElementLimit();
        اگر (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = درست;  // VL_RESTORER in visit(AstNodeAssign)
            بازگشت نادرست;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            اگر (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                اگر (!hasSc && lhsp->dtypep()->skipRefp()->same_درخت(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = درست;  // VL_RESTORER in visit(AstNodeAssign)
                    بازگشت نادرست;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_گره_Assign* newlistp = nullptr;
        برای (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_گره_Assign* ثابت newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_گره::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        بازگشت درست;
    }

    تهی visit(Ast_گره_Assign* nodep) override {
        // Called recursively on newly created assignments
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = نادرست;
        m_assignp = nodep;
        اگر (assignOptimize(nodep)) بازگشت;
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_گره_Biop>
    تهی expandBiOp(T_گره_Biop* biopp) {
        Ast_گره_Biop* nodep = biopp;
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ثابت Ast_گره_DType* ثابت fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_گره_Biop* logp = nullptr;

            ثابت صحیح elements = adtypep->rangep()->elementsConst();
            برای (صحیح elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_گره_Biop* ثابت clonep = جدید T_گره_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                اگر (!logp) {
                    logp = clonep;
                } وگرنه {
                    انتخاب (nodep->type()) {
                    حالت VNType::Eq:  // FALLTHRU
                    حالت VNType::EqCase:
                        logp = جدید AstLogAnd{nodep->fileline(), logp, clonep};
                        بشکن;
                    حالت VNType::Neq:  // FALLTHRU
                    حالت VNType::NeqCase:
                        logp = جدید AstLogOr{nodep->fileline(), logp, clonep};
                        بشکن;
                    پیشفرض: nodep->v3fatalSrc("Unknown node type processing array slice"); بشکن;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    تهی visit(AstEq* nodep) override { expandBiOp(nodep); }
    تهی visit(AstNeq* nodep) override { expandBiOp(nodep); }
    تهی visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    تهی visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    تهی visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ثابت lop = جدید AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ثابت newp
            = جدید AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(درست);
        newp->protect(نادرست);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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

تهی V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("slice", 0, dump_درخت_Either_Level() >= 3);
}
