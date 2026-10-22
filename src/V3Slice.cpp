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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Slice.h"

#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

வகுப்பு SliceVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    மாறிலி VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை* m_assignp = nullptr;  // Assignment we are under
    தருக்க m_assignError = பொய்;  // True if the current assign already has an error
    தருக்க m_okConsArray = பொய்;  // Allow ConsArray children

    // METHODS
    Ast_முனை_கோவை* cloneAndSel(Ast_முனை_கோவை* மாறிலி nodep, uint64_t elements, uint64_t elemIdx,
                             மாறிலி தருக்க needPure) {
        // Insert an ArraySel, except for a few special cases
        மாறிலி AstUnpackArrayDType* மாறிலி arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        எனில் (!arrayp) {  // V3Width should have complained, but...
            எனில் (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } இல்லையெனில் {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = உண்மை;
            // Likely will cause downstream errors
            திரும்பு nodep->clone_மரம்(பொய், needPure);
        }
        எனில் (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            எனில் (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = உண்மை;
            elements = 1;
            elemIdx = 0;
        }
        Ast_முனை_கோவை* newp;
        எனில் (AstConsArray* மாறிலி consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            மாறிலி uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_மரம்(பொய், needPure);
        } இல்லையெனில் எனில் (மாறிலி AstCvtUnpackToUnpack* மாறிலி cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_முனை_கோவை* மாறிலி elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_முனை_DType* மாறிலி subDTypep = arrayp->subDTypep();
            மாறிலி AstUnpackArrayDType* மாறிலி subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            எனில் (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) திரும்பு elemp;
            திரும்பு புதிய AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } இல்லையெனில் எனில் (AstCond* மாறிலி snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            திரும்பு புதிய AstCond{snodep->fileline(), snodep->condp()->clone_மரம்(பொய், needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } இல்லையெனில் எனில் (மாறிலி AstSliceSel* மாறிலி snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            மாறிலி முழு leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = புதிய AstArraySel{nodep->fileline(), snodep->fromp()->clone_மரம்(பொய், needPure),
                                   leOffset};
        } இல்லையெனில் எனில் (மாறிலி AstSampled* மாறிலி snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_முனை_கோவை* மாறிலி exprp = VN_AS(snodep->exprp(), முனை_கோவை);
            Ast_முனை_கோவை* மாறிலி selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            திரும்பு புதிய AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } இல்லையெனில் எனில் (Ast_கோவை_Stmt* மாறிலி snodep = VN_CAST(nodep, கோவை_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_முனை_கோவை* மாறிலி resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            எனில் (snodep->stmtsp()) {
                திரும்பு புதிய Ast_கோவை_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } இல்லையெனில் {
                திரும்பு resultSelp;
            }
        } இல்லையெனில் எனில் (VN_IS(nodep, முனை_Var_Ref) || VN_IS(nodep, முனை_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            மாறிலி முழு leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = புதிய AstArraySel{nodep->fileline(), nodep->clone_மரம்(பொய், needPure), leOffset};
        } இல்லையெனில் {
            எனில் (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = உண்மை;
            // Likely will cause downstream errors
            newp = nodep->clone_மரம்(பொய், needPure);
        }
        திரும்பு newp;
    }

    தருக்க assignOptimize(Ast_முனை_Assign* nodep) {
        // Return true if did optimization
        Ast_முனை_DType* மாறிலி dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_முனை* stp = nodep->rhsp();
        மாறிலி AstUnpackArrayDType* மாறிலி arrayp = VN_CAST(dtp, UnpackArrayDType);
        எனில் (!arrayp) திரும்பு பொய்;
        எனில் (VN_IS(stp, CvtPackedToArray)) திரும்பு பொய்;
        எனில் (VN_IS(stp, CReset)) திரும்பு பொய்;

        // Any isSc variables must be expanded regardless of --fno-slice
        மாறிலி தருக்க hasSc
            = nodep->exists([&](மாறிலி AstVarRef* refp) -> தருக்க { திரும்பு refp->varp()->isSc(); });
        எனில் (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = உண்மை;  // VL_RESTORER in visit(AstNodeAssign)
            திரும்பு பொய்;
        }

        // Skip optimization if array is too large
        மாறிலி முழு elements = arrayp->rangep()->elementsConst();
        மாறிலி முழு elementLimit = v3Global.opt.fSliceElementLimit();
        எனில் (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = உண்மை;  // VL_RESTORER in visit(AstNodeAssign)
            திரும்பு பொய்;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        எனில் (AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            எனில் (AstVarRef* மாறிலி rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                எனில் (!hasSc && lhsp->dtypep()->skipRefp()->same_மரம்(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = உண்மை;  // VL_RESTORER in visit(AstNodeAssign)
                    திரும்பு பொய்;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_முனை_Assign* newlistp = nullptr;
        க்கு (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_முனை_Assign* மாறிலி newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_முனை::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        திரும்பு உண்மை;
    }

    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // Called recursively on newly created assignments
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = பொய்;
        m_assignp = nodep;
        எனில் (assignOptimize(nodep)) திரும்பு;
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_முனை_Biop>
    வெறுமை expandBiOp(T_முனை_Biop* biopp) {
        Ast_முனை_Biop* nodep = biopp;
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        மாறிலி Ast_முனை_DType* மாறிலி fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_முனை_Biop* logp = nullptr;

            மாறிலி முழு elements = adtypep->rangep()->elementsConst();
            க்கு (முழு elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_முனை_Biop* மாறிலி clonep = புதிய T_முனை_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                எனில் (!logp) {
                    logp = clonep;
                } இல்லையெனில் {
                    தேர்வு (nodep->type()) {
                    நிலை VNType::Eq:  // FALLTHRU
                    நிலை VNType::EqCase:
                        logp = புதிய AstLogAnd{nodep->fileline(), logp, clonep};
                        நிறுத்து;
                    நிலை VNType::Neq:  // FALLTHRU
                    நிலை VNType::NeqCase:
                        logp = புதிய AstLogOr{nodep->fileline(), logp, clonep};
                        நிறுத்து;
                    இயல்பு: nodep->v3fatalSrc("Unknown node type processing array slice"); நிறுத்து;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    வெறுமை visit(AstEq* nodep) override { expandBiOp(nodep); }
    வெறுமை visit(AstNeq* nodep) override { expandBiOp(nodep); }
    வெறுமை visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    வெறுமை visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    வெறுமை visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_முனை_கோவை* மாறிலி fromp = nodep->fromp()->unlinkFrBack();
        AstConst* மாறிலி lop = புதிய AstConst{nodep->fileline(), AstConst::Widthed_மதிப்பு{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* மாறிலி newp
            = புதிய AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(உண்மை);
        newp->protect(பொய்);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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

வெறுமை V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("slice", 0, dump_மரம்_Either_Level() >= 3);
}
