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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Slice.h"

#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

वर्ग SliceVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ध्रुव VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठ* m_assignp = nullptr;  // Assignment we are under
    तार्किक m_assignError = असत्य;  // True if the current assign already has an error
    तार्किक m_okConsArray = असत्य;  // Allow ConsArray children

    // METHODS
    Ast_गाँठ_व्यंजक* cloneAndSel(Ast_गाँठ_व्यंजक* ध्रुव nodep, uint64_t elements, uint64_t elemIdx,
                             ध्रुव तार्किक needPure) {
        // Insert an ArraySel, except for a few special cases
        ध्रुव AstUnpackArrayDType* ध्रुव arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (!arrayp) {  // V3Width should have complained, but...
            यदि (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } अन्यथा {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = सत्य;
            // Likely will cause downstream errors
            प्रत्यागम nodep->clone_वृक्ष(असत्य, needPure);
        }
        यदि (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            यदि (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = सत्य;
            elements = 1;
            elemIdx = 0;
        }
        Ast_गाँठ_व्यंजक* newp;
        यदि (AstConsArray* ध्रुव consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ध्रुव uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_वृक्ष(असत्य, needPure);
        } अन्यथा यदि (ध्रुव AstCvtUnpackToUnpack* ध्रुव cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_गाँठ_व्यंजक* ध्रुव elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_गाँठ_DType* ध्रुव subDTypep = arrayp->subDTypep();
            ध्रुव AstUnpackArrayDType* ध्रुव subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            यदि (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) प्रत्यागम elemp;
            प्रत्यागम नूतन AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } अन्यथा यदि (AstCond* ध्रुव snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            प्रत्यागम नूतन AstCond{snodep->fileline(), snodep->condp()->clone_वृक्ष(असत्य, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } अन्यथा यदि (ध्रुव AstSliceSel* ध्रुव snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ध्रुव पूर्णाङ्क leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = नूतन AstArraySel{nodep->fileline(), snodep->fromp()->clone_वृक्ष(असत्य, needPure),
                                   leOffset};
        } अन्यथा यदि (ध्रुव AstSampled* ध्रुव snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_गाँठ_व्यंजक* ध्रुव exprp = VN_AS(snodep->exprp(), गाँठ_व्यंजक);
            Ast_गाँठ_व्यंजक* ध्रुव selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            प्रत्यागम नूतन AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } अन्यथा यदि (Ast_व्यंजक_Stmt* ध्रुव snodep = VN_CAST(nodep, व्यंजक_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_गाँठ_व्यंजक* ध्रुव resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            यदि (snodep->stmtsp()) {
                प्रत्यागम नूतन Ast_व्यंजक_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } अन्यथा {
                प्रत्यागम resultSelp;
            }
        } अन्यथा यदि (VN_IS(nodep, गाँठ_Var_Ref) || VN_IS(nodep, गाँठ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ध्रुव पूर्णाङ्क leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = नूतन AstArraySel{nodep->fileline(), nodep->clone_वृक्ष(असत्य, needPure), leOffset};
        } अन्यथा {
            यदि (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = सत्य;
            // Likely will cause downstream errors
            newp = nodep->clone_वृक्ष(असत्य, needPure);
        }
        प्रत्यागम newp;
    }

    तार्किक assignOptimize(Ast_गाँठ_Assign* nodep) {
        // Return true if did optimization
        Ast_गाँठ_DType* ध्रुव dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_गाँठ* stp = nodep->rhsp();
        ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtp, UnpackArrayDType);
        यदि (!arrayp) प्रत्यागम असत्य;
        यदि (VN_IS(stp, CvtPackedToArray)) प्रत्यागम असत्य;
        यदि (VN_IS(stp, CReset)) प्रत्यागम असत्य;

        // Any isSc variables must be expanded regardless of --fno-slice
        ध्रुव तार्किक hasSc
            = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक { प्रत्यागम refp->varp()->isSc(); });
        यदि (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = सत्य;  // VL_RESTORER in visit(AstNodeAssign)
            प्रत्यागम असत्य;
        }

        // Skip optimization if array is too large
        ध्रुव पूर्णाङ्क elements = arrayp->rangep()->elementsConst();
        ध्रुव पूर्णाङ्क elementLimit = v3Global.opt.fSliceElementLimit();
        यदि (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = सत्य;  // VL_RESTORER in visit(AstNodeAssign)
            प्रत्यागम असत्य;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        यदि (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            यदि (AstVarRef* ध्रुव rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                यदि (!hasSc && lhsp->dtypep()->skipRefp()->same_वृक्ष(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = सत्य;  // VL_RESTORER in visit(AstNodeAssign)
                    प्रत्यागम असत्य;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_गाँठ_Assign* newlistp = nullptr;
        प्रति (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_गाँठ_Assign* ध्रुव newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_गाँठ::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        प्रत्यागम सत्य;
    }

    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        // Called recursively on newly created assignments
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = असत्य;
        m_assignp = nodep;
        यदि (assignOptimize(nodep)) प्रत्यागम;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    template <typename T_गाँठ_Biop>
    शून्य expandBiOp(T_गाँठ_Biop* biopp) {
        Ast_गाँठ_Biop* nodep = biopp;
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ध्रुव Ast_गाँठ_DType* ध्रुव fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_गाँठ_Biop* logp = nullptr;

            ध्रुव पूर्णाङ्क elements = adtypep->rangep()->elementsConst();
            प्रति (पूर्णाङ्क elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_गाँठ_Biop* ध्रुव clonep = नूतन T_गाँठ_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                यदि (!logp) {
                    logp = clonep;
                } अन्यथा {
                    विकल्प (nodep->type()) {
                    पक्ष VNType::Eq:  // FALLTHRU
                    पक्ष VNType::EqCase:
                        logp = नूतन AstLogAnd{nodep->fileline(), logp, clonep};
                        भङ्ग;
                    पक्ष VNType::Neq:  // FALLTHRU
                    पक्ष VNType::NeqCase:
                        logp = नूतन AstLogOr{nodep->fileline(), logp, clonep};
                        भङ्ग;
                    स्वभाव: nodep->v3fatalSrc("Unknown node type processing array slice"); भङ्ग;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    शून्य visit(AstEq* nodep) override { expandBiOp(nodep); }
    शून्य visit(AstNeq* nodep) override { expandBiOp(nodep); }
    शून्य visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    शून्य visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    शून्य visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_गाँठ_व्यंजक* ध्रुव fromp = nodep->fromp()->unlinkFrBack();
        AstConst* ध्रुव lop = नूतन AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ध्रुव newp
            = नूतन AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(सत्य);
        newp->protect(असत्य);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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

शून्य V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("slice", 0, dump_वृक्ष_Either_Level() >= 3);
}
