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

صنف ټوټه_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_stat_ټوټه_Element_Skips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه* m_assignp = تش_اشاره;  // Assignment we are under
    منطقي m_ټاکل_Error = خطأ;  // True if the current assign already has an error
    منطقي m_okConsArray = خطأ;  // Allow ConsArray children

    // METHODS
    Ast_غوټه_بیان* cloneAndSel(Ast_غوټه_بیان* ثابت nodep, uint64_t elements, uint64_t elemIdx,
                             ثابت منطقي need_خالص) {
        // Insert an ArraySel, except for a few special cases
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (!arrayp) {  // V3Width should have complained, but...
            إذا (!m_ټاکل_Error) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } وإلا {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_ټاکل_Error = ریښتیا;
            // Likely will cause downstream errors
            أرجع nodep->clone_ونه(خطأ, need_خالص);
        }
        إذا (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            إذا (!m_ټاکل_Error) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_ټاکل_Error = ریښتیا;
            elements = 1;
            elemIdx = 0;
        }
        Ast_غوټه_بیان* newp;
        إذا (AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ثابت uint64_t شاخص
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(شاخص)->clone_ونه(خطأ, need_خالص);
        } وإلا إذا (ثابت Ast_Cvt_Unpack_منزل_Unpack* ثابت cvtp = VN_CAST(nodep, Cvt_Unpack_منزل_Unpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_غوټه_بیان* ثابت elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, need_خالص);
            Ast_غوټه_DType* ثابت subDTypep = arrayp->subDTypep();
            ثابت AstUnpackArrayDType* ثابت subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            إذا (!subArrayp || !subArrayp->is_سرچپه_د(elemp->dtypep())) أرجع elemp;
            أرجع جديد Ast_Cvt_Unpack_منزل_Unpack{nodep->fileline(), elemp, subDTypep};
        } وإلا إذا (AstCond* ثابت snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            أرجع جديد AstCond{snodep->fileline(), snodep->condp()->clone_ونه(خطأ, need_خالص),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, need_خالص),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, need_خالص)};
        } وإلا إذا (ثابت Ast_ټوټه_Sel* ثابت snodep = VN_CAST(nodep, ټوټه_Sel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحيح le_دننه_واټن = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = جديد AstArraySel{nodep->fileline(), snodep->fromp()->clone_ونه(خطأ, need_خالص),
                                   le_دننه_واټن};
        } وإلا إذا (ثابت AstSampled* ثابت snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_غوټه_بیان* ثابت exprp = VN_AS(snodep->exprp(), غوټه_بیان);
            Ast_غوټه_بیان* ثابت selp = cloneAndSel(exprp, elements, elemIdx, need_خالص);
            أرجع جديد AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->دننی_2()};
        } وإلا إذا (Ast_بیان_Stmt* ثابت snodep = VN_CAST(nodep, بیان_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_غوټه_بیان* ثابت پایله_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, need_خالص);
            إذا (snodep->stmtsp()) {
                أرجع جديد Ast_بیان_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_شا_With_بل(),
                                       پایله_Selp};
            } وإلا {
                أرجع پایله_Selp;
            }
        } وإلا إذا (VN_IS(nodep, غوټه_Var_Ref) || VN_IS(nodep, غوټه_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحيح le_دننه_واټن = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = جديد AstArraySel{nodep->fileline(), nodep->clone_ونه(خطأ, need_خالص), le_دننه_واټن};
        } وإلا {
            إذا (!m_ټاکل_Error) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_ټاکل_Error = ریښتیا;
            // Likely will cause downstream errors
            newp = nodep->clone_ونه(خطأ, need_خالص);
        }
        أرجع newp;
    }

    منطقي ټاکل_Optimize(Ast_غوټه_ټاکل* nodep) {
        // Return true if did optimization
        Ast_غوټه_DType* ثابت dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_غوټه* stp = nodep->rhsp();
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtp, UnpackArrayDType);
        إذا (!arrayp) أرجع خطأ;
        إذا (VN_IS(stp, Cvt_Packed_منزل_Array)) أرجع خطأ;
        إذا (VN_IS(stp, CReset)) أرجع خطأ;

        // Any isSc variables must be expanded regardless of --fno-slice
        ثابت منطقي hasSc
            = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي { أرجع refp->varp()->isSc(); });
        إذا (!hasSc && !v3Global.opt.f_ټوټه()) {
            m_okConsArray = ریښتیا;  // VL_RESTORER in visit(AstNodeAssign)
            أرجع خطأ;
        }

        // Skip optimization if array is too large
        ثابت صحيح elements = arrayp->rangep()->elementsConst();
        ثابت صحيح element_حد = v3Global.opt.f_ټوټه_Element_حد();
        إذا (elements > element_حد && element_حد > 0) {
            ++m_stat_ټوټه_Element_Skips;
            m_okConsArray = ریښتیا;  // VL_RESTORER in visit(AstNodeAssign)
            أرجع خطأ;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                إذا (!hasSc && lhsp->dtypep()->skipRefp()->same_ونه(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = ریښتیا;  // VL_RESTORER in visit(AstNodeAssign)
                    أرجع خطأ;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_غوټه_ټاکل* newlistp = تش_اشاره;
        لكل (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_غوټه_ټاکل* ثابت newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_غوټه::جمع_کړه_بل(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        أرجع ریښتیا;
    }

    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        // Called recursively on newly created assignments
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_ټاکل_Error);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_ټاکل_Error = خطأ;
        m_assignp = nodep;
        إذا (ټاکل_Optimize(nodep)) أرجع;
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    کينډۍ <ډول_نوم T_غوټه_Biop>
    فراغ expandBiOp(T_غوټه_Biop* biopp) {
        Ast_غوټه_Biop* nodep = biopp;
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ثابت Ast_غوټه_DType* ثابت fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_غوټه_Biop* logp = تش_اشاره;

            ثابت صحيح elements = adtypep->rangep()->elementsConst();
            لكل (صحيح elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_غوټه_Biop* ثابت clonep = جديد T_غوټه_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                إذا (!logp) {
                    logp = clonep;
                } وإلا {
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
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    فراغ لیدنه(AstEq* nodep) override { expandBiOp(nodep); }
    فراغ لیدنه(AstNeq* nodep) override { expandBiOp(nodep); }
    فراغ لیدنه(AstEqCase* nodep) override { expandBiOp(nodep); }
    فراغ لیدنه(AstNeqCase* nodep) override { expandBiOp(nodep); }

    فراغ لیدنه(Ast_ټوټه_Sel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_غوټه_بیان* ثابت fromp = nodep->fromp()->unlink_Fr_شا();
        AstConst* ثابت lop = جديد AstConst{nodep->fileline(), AstConst::Widthed_ارزښت{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ثابت newp
            = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_ټوټه};
        newp->جمع_کړه_Pinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->did_پلنوالی(ریښتیا);
        newp->protect(خطأ);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ټوټه_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ټوټه_Visitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::جمع_کړه_Stat("Optimizations, Slice array skips due to size limit",
                         m_stat_ټوټه_Element_Skips);
    }
};

//######################################################################
// Link class functions

فراغ V3_ټوټه::ټوټه_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ټوټه_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("slice", 0, dump_ونه_Either_Level() >= 3);
}
