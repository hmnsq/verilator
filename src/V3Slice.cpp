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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Slice.h"

#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

વર્ગ SliceVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    અચલ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ* m_assignp = nullptr;  // Assignment we are under
    તાર્કિક m_assignError = ખોટું;  // True if the current assign already has an error
    તાર્કિક m_okConsArray = ખોટું;  // Allow ConsArray children

    // METHODS
    Ast_ગાંઠ_પદાવલિ* cloneAndSel(Ast_ગાંઠ_પદાવલિ* અચલ nodep, uint64_t elements, uint64_t elemIdx,
                             અચલ તાર્કિક needPure) {
        // Insert an ArraySel, except for a few special cases
        અચલ AstUnpackArrayDType* અચલ arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        જો (!arrayp) {  // V3Width should have complained, but...
            જો (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } નહીંતર {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = સાચું;
            // Likely will cause downstream errors
            પાછા nodep->clone_વૃક્ષ(ખોટું, needPure);
        }
        જો (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            જો (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = સાચું;
            elements = 1;
            elemIdx = 0;
        }
        Ast_ગાંઠ_પદાવલિ* newp;
        જો (AstConsArray* અચલ consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            અચલ uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_વૃક્ષ(ખોટું, needPure);
        } નહીંતર જો (અચલ AstCvtUnpackToUnpack* અચલ cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_ગાંઠ_પદાવલિ* અચલ elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_ગાંઠ_DType* અચલ subDTypep = arrayp->subDTypep();
            અચલ AstUnpackArrayDType* અચલ subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            જો (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) પાછા elemp;
            પાછા નવું AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } નહીંતર જો (AstCond* અચલ snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            પાછા નવું AstCond{snodep->fileline(), snodep->condp()->clone_વૃક્ષ(ખોટું, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } નહીંતર જો (અચલ AstSliceSel* અચલ snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            અચલ પૂર્ણાંક leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = નવું AstArraySel{nodep->fileline(), snodep->fromp()->clone_વૃક્ષ(ખોટું, needPure),
                                   leOffset};
        } નહીંતર જો (અચલ AstSampled* અચલ snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_AS(snodep->exprp(), ગાંઠ_પદાવલિ);
            Ast_ગાંઠ_પદાવલિ* અચલ selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            પાછા નવું AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } નહીંતર જો (Ast_પદાવલિ_Stmt* અચલ snodep = VN_CAST(nodep, પદાવલિ_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ગાંઠ_પદાવલિ* અચલ resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            જો (snodep->stmtsp()) {
                પાછા નવું Ast_પદાવલિ_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } નહીંતર {
                પાછા resultSelp;
            }
        } નહીંતર જો (VN_IS(nodep, ગાંઠ_Var_Ref) || VN_IS(nodep, ગાંઠ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            અચલ પૂર્ણાંક leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = નવું AstArraySel{nodep->fileline(), nodep->clone_વૃક્ષ(ખોટું, needPure), leOffset};
        } નહીંતર {
            જો (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = સાચું;
            // Likely will cause downstream errors
            newp = nodep->clone_વૃક્ષ(ખોટું, needPure);
        }
        પાછા newp;
    }

    તાર્કિક assignOptimize(Ast_ગાંઠ_Assign* nodep) {
        // Return true if did optimization
        Ast_ગાંઠ_DType* અચલ dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_ગાંઠ* stp = nodep->rhsp();
        અચલ AstUnpackArrayDType* અચલ arrayp = VN_CAST(dtp, UnpackArrayDType);
        જો (!arrayp) પાછા ખોટું;
        જો (VN_IS(stp, CvtPackedToArray)) પાછા ખોટું;
        જો (VN_IS(stp, CReset)) પાછા ખોટું;

        // Any isSc variables must be expanded regardless of --fno-slice
        અચલ તાર્કિક hasSc
            = nodep->exists([&](અચલ AstVarRef* refp) -> તાર્કિક { પાછા refp->varp()->isSc(); });
        જો (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = સાચું;  // VL_RESTORER in visit(AstNodeAssign)
            પાછા ખોટું;
        }

        // Skip optimization if array is too large
        અચલ પૂર્ણાંક elements = arrayp->rangep()->elementsConst();
        અચલ પૂર્ણાંક elementLimit = v3Global.opt.fSliceElementLimit();
        જો (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = સાચું;  // VL_RESTORER in visit(AstNodeAssign)
            પાછા ખોટું;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        જો (AstVarRef* અચલ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            જો (AstVarRef* અચલ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                જો (!hasSc && lhsp->dtypep()->skipRefp()->same_વૃક્ષ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = સાચું;  // VL_RESTORER in visit(AstNodeAssign)
                    પાછા ખોટું;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_ગાંઠ_Assign* newlistp = nullptr;
        માટે (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_ગાંઠ_Assign* અચલ newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_ગાંઠ::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        પાછા સાચું;
    }

    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // Called recursively on newly created assignments
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = ખોટું;
        m_assignp = nodep;
        જો (assignOptimize(nodep)) પાછા;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_ગાંઠ_Biop>
    શૂન્ય expandBiOp(T_ગાંઠ_Biop* biopp) {
        Ast_ગાંઠ_Biop* nodep = biopp;
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        અચલ Ast_ગાંઠ_DType* અચલ fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        જો (અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_ગાંઠ_Biop* logp = nullptr;

            અચલ પૂર્ણાંક elements = adtypep->rangep()->elementsConst();
            માટે (પૂર્ણાંક elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_ગાંઠ_Biop* અચલ clonep = નવું T_ગાંઠ_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                જો (!logp) {
                    logp = clonep;
                } નહીંતર {
                    પસંદગી (nodep->type()) {
                    સ્થિતિ VNType::Eq:  // FALLTHRU
                    સ્થિતિ VNType::EqCase:
                        logp = નવું AstLogAnd{nodep->fileline(), logp, clonep};
                        તોડો;
                    સ્થિતિ VNType::Neq:  // FALLTHRU
                    સ્થિતિ VNType::NeqCase:
                        logp = નવું AstLogOr{nodep->fileline(), logp, clonep};
                        તોડો;
                    મૂળભૂત: nodep->v3fatalSrc("Unknown node type processing array slice"); તોડો;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    શૂન્ય visit(AstEq* nodep) override { expandBiOp(nodep); }
    શૂન્ય visit(AstNeq* nodep) override { expandBiOp(nodep); }
    શૂન્ય visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    શૂન્ય visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    શૂન્ય visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = nodep->fromp()->unlinkFrBack();
        AstConst* અચલ lop = નવું AstConst{nodep->fileline(), AstConst::Widthed_કિંમત{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* અચલ newp
            = નવું AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(સાચું);
        newp->protect(ખોટું);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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

શૂન્ય V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("slice", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
