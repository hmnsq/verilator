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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Slice.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

ਵਰਗ ਟੁਕੜਾ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_stat_ਟੁਕੜਾ_Element_Skips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ* m_assignp = ਖਾਲੀ_ਸੂਚਕ;  // Assignment we are under
    ਤਾਰਕਿਕ m_ਸੌਂਪੋ_Error = ਝੂਠ;  // True if the current assign already has an error
    ਤਾਰਕਿਕ m_okConsArray = ਝੂਠ;  // Allow ConsArray children

    // METHODS
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* cloneAndSel(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep, uint64_t elements, uint64_t elemIdx,
                             ਅਚਲ ਤਾਰਕਿਕ need_ਸ਼ੁੱਧ) {
        // Insert an ArraySel, except for a few special cases
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        ਜੇ (!arrayp) {  // V3Width should have complained, but...
            ਜੇ (!m_ਸੌਂਪੋ_Error) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } ਨਹੀਂਤਾਂ {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_ਸੌਂਪੋ_Error = ਸੱਚ;
            // Likely will cause downstream errors
            ਵਾਪਸ nodep->clone_ਰੁੱਖ(ਝੂਠ, need_ਸ਼ੁੱਧ);
        }
        ਜੇ (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            ਜੇ (!m_ਸੌਂਪੋ_Error) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_ਸੌਂਪੋ_Error = ਸੱਚ;
            elements = 1;
            elemIdx = 0;
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
        ਜੇ (AstConsArray* ਅਚਲ consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ਅਚਲ uint64_t ਸੂਚਕਾਂਕ
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(ਸੂਚਕਾਂਕ)->clone_ਰੁੱਖ(ਝੂਠ, need_ਸ਼ੁੱਧ);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack* ਅਚਲ cvtp = VN_CAST(nodep, Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, need_ਸ਼ੁੱਧ);
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = arrayp->subDTypep();
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            ਜੇ (!subArrayp || !subArrayp->is_ਉਲਟ_ਦਾ(elemp->dtypep())) ਵਾਪਸ elemp;
            ਵਾਪਸ ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{nodep->fileline(), elemp, subDTypep};
        } ਨਹੀਂਤਾਂ ਜੇ (AstCond* ਅਚਲ snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            ਵਾਪਸ ਨਵਾਂ AstCond{snodep->fileline(), snodep->condp()->clone_ਰੁੱਖ(ਝੂਠ, need_ਸ਼ੁੱਧ),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, need_ਸ਼ੁੱਧ),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, need_ਸ਼ੁੱਧ)};
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਟੁਕੜਾ_Sel* ਅਚਲ snodep = VN_CAST(nodep, ਟੁਕੜਾ_Sel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ਅਚਲ ਪੂਰਨ le_ਵਿੱਥ = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = ਨਵਾਂ AstArraySel{nodep->fileline(), snodep->fromp()->clone_ਰੁੱਖ(ਝੂਠ, need_ਸ਼ੁੱਧ),
                                   le_ਵਿੱਥ};
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSampled* ਅਚਲ snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_AS(snodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ selp = cloneAndSel(exprp, elements, elemIdx, need_ਸ਼ੁੱਧ);
            ਵਾਪਸ ਨਵਾਂ AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->ਅੰਦਰੂਨੀ_2()};
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ snodep = VN_CAST(nodep, ਵਿਅੰਜਕ_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਨਤੀਜਾ_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, need_ਸ਼ੁੱਧ);
            ਜੇ (snodep->stmtsp()) {
                ਵਾਪਸ ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(),
                                       ਨਤੀਜਾ_Selp};
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਨਤੀਜਾ_Selp;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, ਗੰਢ_Var_Ref) || VN_IS(nodep, ਗੰਢ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ਅਚਲ ਪੂਰਨ le_ਵਿੱਥ = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = ਨਵਾਂ AstArraySel{nodep->fileline(), nodep->clone_ਰੁੱਖ(ਝੂਠ, need_ਸ਼ੁੱਧ), le_ਵਿੱਥ};
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!m_ਸੌਂਪੋ_Error) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_ਸੌਂਪੋ_Error = ਸੱਚ;
            // Likely will cause downstream errors
            newp = nodep->clone_ਰੁੱਖ(ਝੂਠ, need_ਸ਼ੁੱਧ);
        }
        ਵਾਪਸ newp;
    }

    ਤਾਰਕਿਕ ਸੌਂਪੋ_Optimize(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        // Return true if did optimization
        Ast_ਗੰਢ_DType* ਅਚਲ dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_ਗੰਢ* stp = nodep->rhsp();
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp = VN_CAST(dtp, UnpackArrayDType);
        ਜੇ (!arrayp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_IS(stp, Cvt_Packed_ਮੰਜ਼ਿਲ_Array)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_IS(stp, CReset)) ਵਾਪਸ ਝੂਠ;

        // Any isSc variables must be expanded regardless of --fno-slice
        ਅਚਲ ਤਾਰਕਿਕ hasSc
            = nodep->exists([&](ਅਚਲ AstVarRef* refp) -> ਤਾਰਕਿਕ { ਵਾਪਸ refp->varp()->isSc(); });
        ਜੇ (!hasSc && !v3Global.opt.f_ਟੁਕੜਾ()) {
            m_okConsArray = ਸੱਚ;  // VL_RESTORER in visit(AstNodeAssign)
            ਵਾਪਸ ਝੂਠ;
        }

        // Skip optimization if array is too large
        ਅਚਲ ਪੂਰਨ elements = arrayp->rangep()->elementsConst();
        ਅਚਲ ਪੂਰਨ element_ਹੱਦਬੰਦੀ = v3Global.opt.f_ਟੁਕੜਾ_Element_ਹੱਦਬੰਦੀ();
        ਜੇ (elements > element_ਹੱਦਬੰਦੀ && element_ਹੱਦਬੰਦੀ > 0) {
            ++m_stat_ਟੁਕੜਾ_Element_Skips;
            m_okConsArray = ਸੱਚ;  // VL_RESTORER in visit(AstNodeAssign)
            ਵਾਪਸ ਝੂਠ;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        ਜੇ (AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ਜੇ (AstVarRef* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ਜੇ (!hasSc && lhsp->dtypep()->skipRefp()->same_ਰੁੱਖ(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = ਸੱਚ;  // VL_RESTORER in visit(AstNodeAssign)
                    ਵਾਪਸ ਝੂਠ;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_ਗੰਢ_ਸੌਂਪੋ* newlistp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // Called recursively on newly created assignments
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_ਸੌਂਪੋ_Error);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_ਸੌਂਪੋ_Error = ਝੂਠ;
        m_assignp = nodep;
        ਜੇ (ਸੌਂਪੋ_Optimize(nodep)) ਵਾਪਸ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ_Biop>
    ਖਾਲੀ expandBiOp(T_ਗੰਢ_Biop* biopp) {
        Ast_ਗੰਢ_Biop* nodep = biopp;
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_ਗੰਢ_Biop* logp = ਖਾਲੀ_ਸੂਚਕ;

            ਅਚਲ ਪੂਰਨ elements = adtypep->rangep()->elementsConst();
            ਲਈ (ਪੂਰਨ elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_ਗੰਢ_Biop* ਅਚਲ clonep = ਨਵਾਂ T_ਗੰਢ_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                ਜੇ (!logp) {
                    logp = clonep;
                } ਨਹੀਂਤਾਂ {
                    ਚੋਣ (nodep->type()) {
                    ਹਾਲਤ VNType::Eq:  // FALLTHRU
                    ਹਾਲਤ VNType::EqCase:
                        logp = ਨਵਾਂ AstLogAnd{nodep->fileline(), logp, clonep};
                        ਰੋਕੋ;
                    ਹਾਲਤ VNType::Neq:  // FALLTHRU
                    ਹਾਲਤ VNType::NeqCase:
                        logp = ਨਵਾਂ AstLogOr{nodep->fileline(), logp, clonep};
                        ਰੋਕੋ;
                    ਮੂਲ: nodep->v3fatalSrc("Unknown node type processing array slice"); ਰੋਕੋ;
                    }
                }
            }
            UASSERT_OBJ(logp, nodep, "Unpacked array with empty indices range");
            nodep->replaceWith(logp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstEq* nodep) override { expandBiOp(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeq* nodep) override { expandBiOp(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstEqCase* nodep) override { expandBiOp(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqCase* nodep) override { expandBiOp(nodep); }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
        AstConst* ਅਚਲ lop = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ਅਚਲ newp
            = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_ਟੁਕੜਾ};
        newp->ਜੋੜੋ_Pinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->did_ਚੌੜਾਈ(ਸੱਚ);
        newp->protect(ਝੂਠ);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਟੁਕੜਾ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਟੁਕੜਾ_Visitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Slice array skips due to size limit",
                         m_stat_ਟੁਕੜਾ_Element_Skips);
    }
};

//######################################################################
// Link class functions

ਖਾਲੀ V3_ਟੁਕੜਾ::ਟੁਕੜਾ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਟੁਕੜਾ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("slice", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
