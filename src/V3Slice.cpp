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

ವರ್ಗ ತುಂಡು_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_stat_ತುಂಡು_Element_Skips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು* m_assignp = ಶೂನ್ಯಸೂಚಕ;  // Assignment we are under
    ತಾರ್ಕಿಕ m_ನಿಯೋಜಿಸು_Error = ಸುಳ್ಳು;  // True if the current assign already has an error
    ತಾರ್ಕಿಕ m_okConsArray = ಸುಳ್ಳು;  // Allow ConsArray children

    // METHODS
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cloneAndSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep, uint64_t elements, uint64_t elemIdx,
                             ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_ಶುದ್ಧ) {
        // Insert an ArraySel, except for a few special cases
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ಆದರೆ (!arrayp) {  // V3Width should have complained, but...
            ಆದರೆ (!m_ನಿಯೋಜಿಸು_Error) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_ನಿಯೋಜಿಸು_Error = ನಿಜ;
            // Likely will cause downstream errors
            ಹಿಂತಿರುಗಿ nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು, need_ಶುದ್ಧ);
        }
        ಆದರೆ (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            ಆದರೆ (!m_ನಿಯೋಜಿಸು_Error) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_ನಿಯೋಜಿಸು_Error = ನಿಜ;
            elements = 1;
            elemIdx = 0;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
        ಆದರೆ (AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ಸ್ಥಿರಾಂಕ uint64_t ಸೂಚ್ಯಂಕ
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(ಸೂಚ್ಯಂಕ)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು, need_ಶುದ್ಧ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack* ಸ್ಥಿರಾಂಕ cvtp = VN_CAST(nodep, Cvt_Unpack_ಗಮ್ಯ_Unpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, need_ಶುದ್ಧ);
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subDTypep = arrayp->subDTypep();
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            ಆದರೆ (!subArrayp || !subArrayp->is_ಹಿಮ್ಮುಖ_ದ(elemp->dtypep())) ಹಿಂತಿರುಗಿ elemp;
            ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{nodep->fileline(), elemp, subDTypep};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{snodep->fileline(), snodep->condp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು, need_ಶುದ್ಧ),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, need_ಶುದ್ಧ),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, need_ಶುದ್ಧ)};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ತುಂಡು_Sel* ಸ್ಥಿರಾಂಕ snodep = VN_CAST(nodep, ತುಂಡು_Sel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ le_ಸ್ಥಾನಾಂತರ = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = ಹೊಸ AstArraySel{nodep->fileline(), snodep->fromp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು, need_ಶುದ್ಧ),
                                   le_ಸ್ಥಾನಾಂತರ};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSampled* ಸ್ಥಿರಾಂಕ snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_AS(snodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ selp = cloneAndSel(exprp, elements, elemIdx, need_ಶುದ್ಧ);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->ಆಂತರಿಕ_2()};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ snodep = VN_CAST(nodep, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, need_ಶುದ್ಧ);
            ಆದರೆ (snodep->stmtsp()) {
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(),
                                       ಫಲಿತಾಂಶ_Selp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ_Selp;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, ಗಂಟು_Var_Ref) || VN_IS(nodep, ಗಂಟು_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ le_ಸ್ಥಾನಾಂತರ = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = ಹೊಸ AstArraySel{nodep->fileline(), nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು, need_ಶುದ್ಧ), le_ಸ್ಥಾನಾಂತರ};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!m_ನಿಯೋಜಿಸು_Error) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_ನಿಯೋಜಿಸು_Error = ನಿಜ;
            // Likely will cause downstream errors
            newp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು, need_ಶುದ್ಧ);
        }
        ಹಿಂತಿರುಗಿ newp;
    }

    ತಾರ್ಕಿಕ ನಿಯೋಜಿಸು_Optimize(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        // Return true if did optimization
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_ಗಂಟು* stp = nodep->rhsp();
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtp, UnpackArrayDType);
        ಆದರೆ (!arrayp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(stp, Cvt_Packed_ಗಮ್ಯ_Array)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(stp, CReset)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Any isSc variables must be expanded regardless of --fno-slice
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasSc
            = nodep->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) -> ತಾರ್ಕಿಕ { ಹಿಂತಿರುಗಿ refp->varp()->isSc(); });
        ಆದರೆ (!hasSc && !v3Global.opt.f_ತುಂಡು()) {
            m_okConsArray = ನಿಜ;  // VL_RESTORER in visit(AstNodeAssign)
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Skip optimization if array is too large
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements = arrayp->rangep()->elementsConst();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ element_ಎಲ್ಲೆ = v3Global.opt.f_ತುಂಡು_Element_ಎಲ್ಲೆ();
        ಆದರೆ (elements > element_ಎಲ್ಲೆ && element_ಎಲ್ಲೆ > 0) {
            ++m_stat_ತುಂಡು_Element_Skips;
            m_okConsArray = ನಿಜ;  // VL_RESTORER in visit(AstNodeAssign)
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ಆದರೆ (!hasSc && lhsp->dtypep()->skipRefp()->same_ವೃಕ್ಷ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = ನಿಜ;  // VL_RESTORER in visit(AstNodeAssign)
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* newlistp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // Called recursively on newly created assignments
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_ನಿಯೋಜಿಸು_Error);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_ನಿಯೋಜಿಸು_Error = ಸುಳ್ಳು;
        m_assignp = nodep;
        ಆದರೆ (ನಿಯೋಜಿಸು_Optimize(nodep)) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಗಂಟು_Biop>
    ಶೂನ್ಯ expandBiOp(T_ಗಂಟು_Biop* biopp) {
        Ast_ಗಂಟು_Biop* nodep = biopp;
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_ಗಂಟು_Biop* logp = ಶೂನ್ಯಸೂಚಕ;

            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements = adtypep->rangep()->elementsConst();
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ clonep = ಹೊಸ T_ಗಂಟು_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                ಆದರೆ (!logp) {
                    logp = clonep;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆಯ್ಕೆ (nodep->type()) {
                    ಸ್ಥಿತಿ VNType::Eq:  // FALLTHRU
                    ಸ್ಥಿತಿ VNType::EqCase:
                        logp = ಹೊಸ AstLogAnd{nodep->fileline(), logp, clonep};
                        ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ VNType::Neq:  // FALLTHRU
                    ಸ್ಥಿತಿ VNType::NeqCase:
                        logp = ಹೊಸ AstLogOr{nodep->fileline(), logp, clonep};
                        ನಿಲ್ಲಿಸು;
                    ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Unknown node type processing array slice"); ನಿಲ್ಲಿಸು;
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

    ಶೂನ್ಯ ಭೇಟಿ(AstEq* nodep) override { expandBiOp(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeq* nodep) override { expandBiOp(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstEqCase* nodep) override { expandBiOp(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstNeqCase* nodep) override { expandBiOp(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
        AstConst* ಸ್ಥಿರಾಂಕ lop = ಹೊಸ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_ತುಂಡು};
        newp->ಕೂಡಿಸು_Pinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->did_ಅಗಲ(ನಿಜ);
        newp->protect(ಸುಳ್ಳು);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ತುಂಡು_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ತುಂಡು_Visitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Slice array skips due to size limit",
                         m_stat_ತುಂಡು_Element_Skips);
    }
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3_ತುಂಡು::ತುಂಡು_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ತುಂಡು_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("slice", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
