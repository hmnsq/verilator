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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Slice.h"

#समावेश "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

श्रेणी SliceVisitor final : खुला VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    स्थिर VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_statSliceElementSkips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठ* m_assignp = nullptr;  // Assignment we are under
    bool m_assignError = false;  // True if the current assign already has an error
    bool m_okConsArray = false;  // Allow ConsArray children

    // METHODS
    Ast_गाँठ_व्यंजक* cloneAndSel(Ast_गाँठ_व्यंजक* स्थिर nodep, uint64_t elements, uint64_t elemIdx,
                             स्थिर bool needPure) {
        // Insert an ArraySel, except for a few special cases
        स्थिर AstUnpackArrayDType* स्थिर arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        अगर (!arrayp) {  // V3Width should have complained, but...
            अगर (!m_assignError) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } अन्यथा {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_assignError = true;
            // Likely will cause downstream errors
            वापस nodep->clone_वृक्ष(false, needPure);
        }
        अगर (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            अगर (!m_assignError) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_assignError = true;
            elements = 1;
            elemIdx = 0;
        }
        Ast_गाँठ_व्यंजक* newp;
        अगर (AstConsArray* स्थिर consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            स्थिर uint64_t सूचकांक_2
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(सूचकांक_2)->clone_वृक्ष(false, needPure);
        } अन्यथा अगर (स्थिर AstCvtUnpackToUnpack* स्थिर cvtp = VN_CAST(nodep, CvtUnpackToUnpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_गाँठ_व्यंजक* स्थिर elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, needPure);
            Ast_गाँठ_DType* स्थिर subDTypep = arrayp->subDTypep();
            स्थिर AstUnpackArrayDType* स्थिर subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            अगर (!subArrayp || !subArrayp->isReverseOf(elemp->dtypep())) वापस elemp;
            वापस नया AstCvtUnpackToUnpack{nodep->fileline(), elemp, subDTypep};
        } अन्यथा अगर (AstCond* स्थिर snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            वापस नया AstCond{snodep->fileline(), snodep->condp()->clone_वृक्ष(false, needPure),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, needPure),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, needPure)};
        } अन्यथा अगर (स्थिर AstSliceSel* स्थिर snodep = VN_CAST(nodep, SliceSel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            स्थिर पूर्णांक leOffset = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = नया AstArraySel{nodep->fileline(), snodep->fromp()->clone_वृक्ष(false, needPure),
                                   leOffset};
        } अन्यथा अगर (स्थिर AstSampled* स्थिर snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_गाँठ_व्यंजक* स्थिर exprp = VN_AS(snodep->exprp(), गाँठ_व्यंजक);
            Ast_गाँठ_व्यंजक* स्थिर selp = cloneAndSel(exprp, elements, elemIdx, needPure);
            वापस नया AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->internal()};
        } अन्यथा अगर (Ast_व्यंजक_Stmt* स्थिर snodep = VN_CAST(nodep, व्यंजक_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_गाँठ_व्यंजक* स्थिर परिणाम_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, needPure);
            अगर (snodep->stmtsp()) {
                वापस नया Ast_व्यंजक_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_Back_With_अग्र(),
                                       परिणाम_Selp};
            } अन्यथा {
                वापस परिणाम_Selp;
            }
        } अन्यथा अगर (VN_IS(nodep, गाँठ_Var_Ref) || VN_IS(nodep, गाँठ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            स्थिर पूर्णांक leOffset = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = नया AstArraySel{nodep->fileline(), nodep->clone_वृक्ष(false, needPure), leOffset};
        } अन्यथा {
            अगर (!m_assignError) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_assignError = true;
            // Likely will cause downstream errors
            newp = nodep->clone_वृक्ष(false, needPure);
        }
        वापस newp;
    }

    bool assignOptimize(Ast_गाँठ_Assign* nodep) {
        // Return true if did optimization
        Ast_गाँठ_DType* स्थिर dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_गाँठ* stp = nodep->rhsp();
        स्थिर AstUnpackArrayDType* स्थिर arrayp = VN_CAST(dtp, UnpackArrayDType);
        अगर (!arrayp) वापस false;
        अगर (VN_IS(stp, CvtPackedToArray)) वापस false;
        अगर (VN_IS(stp, CReset)) वापस false;

        // Any isSc variables must be expanded regardless of --fno-slice
        स्थिर bool hasSc
            = nodep->exists([&](स्थिर AstVarRef* refp) -> bool { वापस refp->varp()->isSc(); });
        अगर (!hasSc && !v3Global.opt.fSlice()) {
            m_okConsArray = true;  // VL_RESTORER in visit(AstNodeAssign)
            वापस false;
        }

        // Skip optimization if array is too large
        स्थिर पूर्णांक elements = arrayp->rangep()->elementsConst();
        स्थिर पूर्णांक elementLimit = v3Global.opt.fSliceElementLimit();
        अगर (elements > elementLimit && elementLimit > 0) {
            ++m_statSliceElementSkips;
            m_okConsArray = true;  // VL_RESTORER in visit(AstNodeAssign)
            वापस false;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        अगर (AstVarRef* स्थिर lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            अगर (AstVarRef* स्थिर rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                अगर (!hasSc && lhsp->dtypep()->skipRefp()->same_वृक्ष(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = true;  // VL_RESTORER in visit(AstNodeAssign)
                    वापस false;
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
        क्रम (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_गाँठ_Assign* स्थिर newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_गाँठ::add_अग्र(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        वापस true;
    }

    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // Called recursively on newly created assignments
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_assignError);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_assignError = false;
        m_assignp = nodep;
        अगर (assignOptimize(nodep)) वापस;
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = true;
        iterateChildren(nodep);
    }
    व्योम visit(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    ढाँचा <typename T_गाँठ_Biop>
    व्योम expandBiOp(T_गाँठ_Biop* biopp) {
        Ast_गाँठ_Biop* nodep = biopp;
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        स्थिर Ast_गाँठ_DType* स्थिर fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_गाँठ_Biop* logp = nullptr;

            स्थिर पूर्णांक elements = adtypep->rangep()->elementsConst();
            क्रम (पूर्णांक elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_गाँठ_Biop* स्थिर clonep = नया T_गाँठ_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                अगर (!logp) {
                    logp = clonep;
                } अन्यथा {
                    चयन (nodep->type()) {
                    हाल VNType::Eq:  // FALLTHRU
                    हाल VNType::EqCase:
                        logp = नया AstLogAnd{nodep->fileline(), logp, clonep};
                        अवरोध;
                    हाल VNType::Neq:  // FALLTHRU
                    हाल VNType::NeqCase:
                        logp = नया AstLogOr{nodep->fileline(), logp, clonep};
                        अवरोध;
                    शेष: nodep->v3fatalSrc("Unknown node type processing array slice"); अवरोध;
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

    व्योम visit(AstEq* nodep) override { expandBiOp(nodep); }
    व्योम visit(AstNeq* nodep) override { expandBiOp(nodep); }
    व्योम visit(AstEqCase* nodep) override { expandBiOp(nodep); }
    व्योम visit(AstNeqCase* nodep) override { expandBiOp(nodep); }

    व्योम visit(AstSliceSel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_गाँठ_व्यंजक* स्थिर fromp = nodep->fromp()->unlinkFrBack();
        AstConst* स्थिर lop = नया AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* स्थिर newp
            = नया AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_SLICE};
        newp->addPinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->didWidth(true);
        newp->protect(false);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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

व्योम V3Slice::sliceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SliceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("slice", 0, dump_वृक्ष_Either_Level() >= 3);
}
