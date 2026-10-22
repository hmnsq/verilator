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
    स्थिरांक VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाठ* m_assignp = nullptr;  // Assignment we are under
    तार्किक m_assignError = असत्य;  // True if the current assign already has an error
    तार्किक m_okConsArray = असत्य;  // Allow ConsArray children

    // METHODS
    Ast_गाठ_अभिव्यक्ती* cloneAndSel(Ast_गाठ_अभिव्यक्ती* स्थिरांक nodep, uint64_t elements, uint64_t elemIdx,
                             स्थिरांक तार्किक needPure) {
        // Insert an ArraySel, except for a few special cases
        स्थिरांक AstUnpackArrayDType* स्थिरांक arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        जर (!arrayp) {  // V3Width should have complained, but...
            जर (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } अन्यथा {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = सत्य;
            // Likely will cause downstream errors
            परत nodep->clone_वृक्ष(असत्य, needPure);
        }
        जर (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            जर (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = सत्य;
            elements = 1;
            elemIdx = 0;
        }
        Ast_गाठ_अभिव्यक्ती* newp;
        जर (AstConsArray* स्थिरांक consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            स्थिरांक uint64_t index
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(index)->clone_वृक्ष(असत्य, needPure);
        } अन्यथा जर (स्थिरांक AstCvtUnpackToUnpack* स्थिरांक cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_गाठ_अभिव्यक्ती* स्थिरांक elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_गाठ_DType* स्थिरांक subDTypep = arrayp->subDTypep();
            स्थिरांक AstUnpackArrayDType* स्थिरांक subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            जर (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) परत elemp;
            परत नवीन AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } अन्यथा जर (AstCond* स्थिरांक snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            परत नवीन AstCond{snodep->fileline(), snodep->condp()->clone_वृक्ष(असत्य, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } अन्यथा जर (स्थिरांक AstSliceSel* स्थिरांक snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            स्थिरांक पूर्णांक leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = नवीन AstArraySel{nodep->fileline(), snodep->fromp()->clone_वृक्ष(असत्य, needPure),
                                   leOffset};
        } अन्यथा जर (स्थिरांक AstSampled* स्थिरांक snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_AS(snodep->exprp(), गाठ_अभिव्यक्ती);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            परत नवीन AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } अन्यथा जर (Ast_अभिव्यक्ती_Stmt* स्थिरांक snodep = VN_CAST(nodep, अभिव्यक्ती_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक resultSelp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            जर (snodep->stmtsp()) {
                परत नवीन Ast_अभिव्यक्ती_Stmt{nodep->fileline(), snodep->stmtsp()->unlinkFrBackWithNext(),
                                       resultSelp};
            } अन्यथा {
                परत resultSelp;
            }
        } अन्यथा जर (VN_IS(nodep, गाठ_Var_Ref) || VN_IS(nodep, गाठ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            स्थिरांक पूर्णांक leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = नवीन AstArraySel{nodep->fileline(), nodep->clone_वृक्ष(असत्य, needPure), leOffset};
        } अन्यथा {
            जर (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = सत्य;
            // Likely will cause downstream errors
            newp = nodep->clone_वृक्ष(असत्य, needPure);
        }
        परत newp;
    }

    तार्किक assignOptimize(Ast_गाठ_Assign* nodep) {
        // Return true if did optimization
        Ast_गाठ_DType* स्थिरांक dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_गाठ* stp = nodep->rhsp();
        स्थिरांक AstUnpackArrayDType* स्थिरांक arrayp = VN_CAST(dtp, UnpackArrayDType);
        जर (!arrayp) परत असत्य;
        जर (VN_IS(stp, CvtPackedToArray)) परत असत्य;
        जर (VN_IS(stp, CReset)) परत असत्य;

        // Any isSc variables must be expanded regardless of --fno-slice
        स्थिरांक तार्किक hasSc
            = nodep->exists([&](स्थिरांक AstVarRef* refp) -> तार्किक { परत refp->varp()->isSc(); });
        जर (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = सत्य;  // VL_RESTORER in visit(AstNodeAssign)
            परत असत्य;
        }

        // Skip optimization if array is too large
        स्थिरांक पूर्णांक elements = arrayp->rangep()->elementsConst();
        स्थिरांक पूर्णांक elementLimit = v3Global.opt.fSliceElementLimit();
        जर (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = सत्य;  // VL_RESTORER in visit(AstNodeAssign)
            परत असत्य;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        जर (AstVarRef* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            जर (AstVarRef* स्थिरांक rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                जर (!hasSc && lhsp->dtypep()->skipRefp()->same_वृक्ष(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = सत्य;  // VL_RESTORER in visit(AstNodeAssign)
                    परत असत्य;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_गाठ_Assign* newlistp = nullptr;
        साठी (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_गाठ_Assign* स्थिरांक newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_गाठ::addNext(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        परत सत्य;
    }

    शून्य visit(Ast_गाठ_Assign* nodep) override {
        // Called recursively on newly created assignments
        जर (nodep->user1SetOnce()) परत;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = असत्य;
        m_assignp = nodep;
        जर (assignOptimize(nodep)) परत;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    ढाँचा <typename T_गाठ_Biop>
    शून्य expandBiOp(T_गाठ_Biop* biopp) {
        Ast_गाठ_Biop* nodep = biopp;
        जर (nodep->user1SetOnce()) परत;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        स्थिरांक Ast_गाठ_DType* स्थिरांक fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_गाठ_Biop* logp = nullptr;

            स्थिरांक पूर्णांक elements = adtypep->rangep()->elementsConst();
            साठी (पूर्णांक elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_गाठ_Biop* स्थिरांक clonep = नवीन T_गाठ_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                जर (!logp) {
                    logp = clonep;
                } अन्यथा {
                    निवड (nodep->type()) {
                    स्थिती VNType::Eq:  // FALLTHRU
                    स्थिती VNType::EqCase:
                        logp = नवीन AstLogAnd{nodep->fileline(), logp, clonep};
                        थांबा;
                    स्थिती VNType::Neq:  // FALLTHRU
                    स्थिती VNType::NeqCase:
                        logp = नवीन AstLogOr{nodep->fileline(), logp, clonep};
                        थांबा;
                    पूर्वनिर्धारित: nodep->v3fatalSrc("Unknown node type processing array slice"); थांबा;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
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
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = nodep->fromp()->unlinkFrBack();
        AstConst* स्थिरांक lop = नवीन AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* स्थिरांक newp
            = नवीन AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(सत्य);
        newp->protect(असत्य);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

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
