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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Slice.h"

#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

വർഗ്ഗം SliceVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി* m_assignp = nullptr;  // Assignment we are under
    തർക്കം m_assignError = അസത്യം;  // True if the current assign already has an error
    തർക്കം m_okConsArray = അസത്യം;  // Allow ConsArray children

    // METHODS
    Ast_കണ്ണി_വ്യഞ്ജകം* cloneAndSel(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep, uint64_t elements, uint64_t elemIdx,
                             സ്ഥിരം തർക്കം needPure) {
        // Insert an ArraySel, except for a few special cases
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (!arrayp) {  // V3Width should have complained, but...
            എങ്കിൽ (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } അല്ലെങ്കിൽ {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = സത്യം;
            // Likely will cause downstream errors
            തിരികെ nodep->clone_വൃക്ഷം(അസത്യം, needPure);
        }
        എങ്കിൽ (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            എങ്കിൽ (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = സത്യം;
            elements = 1;
            elemIdx = 0;
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* newp;
        എങ്കിൽ (AstConsArray* സ്ഥിരം consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            സ്ഥിരം uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_വൃക്ഷം(അസത്യം, needPure);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCvtUnpackToUnpack* സ്ഥിരം cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = arrayp->subDTypep();
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            എങ്കിൽ (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) തിരികെ elemp;
            തിരികെ പുതിയ AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCond* സ്ഥിരം snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            തിരികെ പുതിയ AstCond{snodep->fileline(), snodep->condp()->clone_വൃക്ഷം(അസത്യം, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSliceSel* സ്ഥിരം snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            സ്ഥിരം പൂർണ്ണസംഖ്യ leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = പുതിയ AstArraySel{nodep->fileline(), snodep->fromp()->clone_വൃക്ഷം(അസത്യം, needPure),
                                   leOffset};
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSampled* സ്ഥിരം snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_AS(snodep->exprp(), കണ്ണി_വ്യഞ്ജകം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            തിരികെ പുതിയ AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം snodep = VN_CAST(nodep, വ്യഞ്ജകം_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            എങ്കിൽ (snodep->stmtsp()) {
                തിരികെ പുതിയ Ast_വ്യഞ്ജകം_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } അല്ലെങ്കിൽ {
                തിരികെ resultSelp;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, കണ്ണി_Var_Ref) || VN_IS(nodep, കണ്ണി_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            സ്ഥിരം പൂർണ്ണസംഖ്യ leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = പുതിയ AstArraySel{nodep->fileline(), nodep->clone_വൃക്ഷം(അസത്യം, needPure), leOffset};
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = സത്യം;
            // Likely will cause downstream errors
            newp = nodep->clone_വൃക്ഷം(അസത്യം, needPure);
        }
        തിരികെ newp;
    }

    തർക്കം assignOptimize(Ast_കണ്ണി_Assign* nodep) {
        // Return true if did optimization
        Ast_കണ്ണി_DType* സ്ഥിരം dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_കണ്ണി* stp = nodep->rhsp();
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtp, UnpackArrayDType);
        എങ്കിൽ (!arrayp) തിരികെ അസത്യം;
        എങ്കിൽ (VN_IS(stp, CvtPackedToArray)) തിരികെ അസത്യം;
        എങ്കിൽ (VN_IS(stp, CReset)) തിരികെ അസത്യം;

        // Any isSc variables must be expanded regardless of --fno-slice
        സ്ഥിരം തർക്കം hasSc
            = nodep->exists([&](സ്ഥിരം AstVarRef* refp) -> തർക്കം { തിരികെ refp->varp()->isSc(); });
        എങ്കിൽ (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = സത്യം;  // VL_RESTORER in visit(AstNodeAssign)
            തിരികെ അസത്യം;
        }

        // Skip optimization if array is too large
        സ്ഥിരം പൂർണ്ണസംഖ്യ elements = arrayp->rangep()->elementsConst();
        സ്ഥിരം പൂർണ്ണസംഖ്യ elementLimit = v3Global.opt.fSliceElementLimit();
        എങ്കിൽ (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = സത്യം;  // VL_RESTORER in visit(AstNodeAssign)
            തിരികെ അസത്യം;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        എങ്കിൽ (AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                എങ്കിൽ (!hasSc && lhsp->dtypep()->skipRefp()->same_വൃക്ഷം(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = സത്യം;  // VL_RESTORER in visit(AstNodeAssign)
                    തിരികെ അസത്യം;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_കണ്ണി_Assign* newlistp = nullptr;
        വേണ്ടി (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_കണ്ണി_Assign* സ്ഥിരം newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_കണ്ണി::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // Called recursively on newly created assignments
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = അസത്യം;
        m_assignp = nodep;
        എങ്കിൽ (assignOptimize(nodep)) തിരികെ;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_കണ്ണി_Biop>
    ശൂന്യം expandBiOp(T_കണ്ണി_Biop* biopp) {
        Ast_കണ്ണി_Biop* nodep = biopp;
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_കണ്ണി_Biop* logp = nullptr;

            സ്ഥിരം പൂർണ്ണസംഖ്യ elements = adtypep->rangep()->elementsConst();
            വേണ്ടി (പൂർണ്ണസംഖ്യ elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_കണ്ണി_Biop* സ്ഥിരം clonep = പുതിയ T_കണ്ണി_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                എങ്കിൽ (!logp) {
                    logp = clonep;
                } അല്ലെങ്കിൽ {
                    തിരഞ്ഞെടുപ്പ് (nodep->type()) {
                    അവസ്ഥ VNType::Eq:  // FALLTHRU
                    അവസ്ഥ VNType::EqCase:
                        logp = പുതിയ AstLogAnd{nodep->fileline(), logp, clonep};
                        നിർത്തുക;
                    അവസ്ഥ VNType::Neq:  // FALLTHRU
                    അവസ്ഥ VNType::NeqCase:
                        logp = പുതിയ AstLogOr{nodep->fileline(), logp, clonep};
                        നിർത്തുക;
                    സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Unknown node type processing array slice"); നിർത്തുക;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstEq* nodep) override { expandBiOp(nodep); }
    ശൂന്യം visit(AstNeq* nodep) override { expandBiOp(nodep); }
    ശൂന്യം visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    ശൂന്യം visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    ശൂന്യം visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp()->unlinkFrBack();
        AstConst* സ്ഥിരം lop = പുതിയ AstConst{nodep->fileline(), AstConst::Widthed_മൂല്യം{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* സ്ഥിരം newp
            = പുതിയ AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(സത്യം);
        newp->protect(അസത്യം);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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

ശൂന്യം V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("slice", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
