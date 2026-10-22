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

صنف ٹکڑا_Visitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_stat_ٹکڑا_Element_Skips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ* m_assignp = خالی_سوچک;  // Assignment we are under
    منطقي m_سونپو_Error = خطأ;  // True if the current assign already has an error
    منطقي m_okConsArray = خطأ;  // Allow ConsArray children

    // METHODS
    Ast_گرہ_اظہاریہ* cloneAndSel(Ast_گرہ_اظہاریہ* ثابت nodep, uint64_t elements, uint64_t elemIdx,
                             ثابت منطقي need_خالص) {
        // Insert an ArraySel, except for a few special cases
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (!arrayp) {  // V3Width should have complained, but...
            إذا (!m_سونپو_Error) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } وإلا {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_سونپو_Error = سچ;
            // Likely will cause downstream errors
            أرجع nodep->clone_درخت(خطأ, need_خالص);
        }
        إذا (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            إذا (!m_سونپو_Error) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_سونپو_Error = سچ;
            elements = 1;
            elemIdx = 0;
        }
        Ast_گرہ_اظہاریہ* newp;
        إذا (AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ثابت uint64_t اشاریہ
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(اشاریہ)->clone_درخت(خطأ, need_خالص);
        } وإلا إذا (ثابت Ast_Cvt_Unpack_منزل_Unpack* ثابت cvtp = VN_CAST(nodep, Cvt_Unpack_منزل_Unpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_گرہ_اظہاریہ* ثابت elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, need_خالص);
            Ast_گرہ_DType* ثابت subDTypep = arrayp->subDTypep();
            ثابت AstUnpackArrayDType* ثابت subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            إذا (!subArrayp || !subArrayp->is_الٹ_دا(elemp->dtypep())) أرجع elemp;
            أرجع جديد Ast_Cvt_Unpack_منزل_Unpack{nodep->fileline(), elemp, subDTypep};
        } وإلا إذا (AstCond* ثابت snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            أرجع جديد AstCond{snodep->fileline(), snodep->condp()->clone_درخت(خطأ, need_خالص),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, need_خالص),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, need_خالص)};
        } وإلا إذا (ثابت Ast_ٹکڑا_Sel* ثابت snodep = VN_CAST(nodep, ٹکڑا_Sel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحيح le_وتھ = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = جديد AstArraySel{nodep->fileline(), snodep->fromp()->clone_درخت(خطأ, need_خالص),
                                   le_وتھ};
        } وإلا إذا (ثابت AstSampled* ثابت snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_گرہ_اظہاریہ* ثابت exprp = VN_AS(snodep->exprp(), گرہ_اظہاریہ);
            Ast_گرہ_اظہاریہ* ثابت selp = cloneAndSel(exprp, elements, elemIdx, need_خالص);
            أرجع جديد AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->اندرونی_2()};
        } وإلا إذا (Ast_اظہاریہ_Stmt* ثابت snodep = VN_CAST(nodep, اظہاریہ_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_گرہ_اظہاریہ* ثابت نتیجہ_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, need_خالص);
            إذا (snodep->stmtsp()) {
                أرجع جديد Ast_اظہاریہ_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا(),
                                       نتیجہ_Selp};
            } وإلا {
                أرجع نتیجہ_Selp;
            }
        } وإلا إذا (VN_IS(nodep, گرہ_Var_Ref) || VN_IS(nodep, گرہ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحيح le_وتھ = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = جديد AstArraySel{nodep->fileline(), nodep->clone_درخت(خطأ, need_خالص), le_وتھ};
        } وإلا {
            إذا (!m_سونپو_Error) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_سونپو_Error = سچ;
            // Likely will cause downstream errors
            newp = nodep->clone_درخت(خطأ, need_خالص);
        }
        أرجع newp;
    }

    منطقي سونپو_Optimize(Ast_گرہ_سونپو* nodep) {
        // Return true if did optimization
        Ast_گرہ_DType* ثابت dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_گرہ* stp = nodep->rhsp();
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtp, UnpackArrayDType);
        إذا (!arrayp) أرجع خطأ;
        إذا (VN_IS(stp, Cvt_Packed_منزل_Array)) أرجع خطأ;
        إذا (VN_IS(stp, CReset)) أرجع خطأ;

        // Any isSc variables must be expanded regardless of --fno-slice
        ثابت منطقي hasSc
            = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي { أرجع refp->varp()->isSc(); });
        إذا (!hasSc && !v3Global.opt.f_ٹکڑا()) {
            m_okConsArray = سچ;  // VL_RESTORER in visit(AstNodeAssign)
            أرجع خطأ;
        }

        // Skip optimization if array is too large
        ثابت صحيح elements = arrayp->rangep()->elementsConst();
        ثابت صحيح element_حد_بندی = v3Global.opt.f_ٹکڑا_Element_حد_بندی();
        إذا (elements > element_حد_بندی && element_حد_بندی > 0) {
            ++m_stat_ٹکڑا_Element_Skips;
            m_okConsArray = سچ;  // VL_RESTORER in visit(AstNodeAssign)
            أرجع خطأ;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                إذا (!hasSc && lhsp->dtypep()->skipRefp()->same_درخت(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = سچ;  // VL_RESTORER in visit(AstNodeAssign)
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
        Ast_گرہ_سونپو* newlistp = خالی_سوچک;
        لكل (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_گرہ_سونپو* ثابت newp
                = nodep->cloneType(cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                                   cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0));
            UINFOTREE(9, newp, "", "new");
            newlistp = Ast_گرہ::جوڑو_اگلا(newlistp, newp);
        }
        UINFOTREE(9, nodep, "", "Deslice-Dn");
        nodep->replaceWith(newlistp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // Normal edit iterator will now iterate on all of the expansion assignments
        // This will potentially call this function again to resolve next level of slicing
        أرجع سچ;
    }

    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // Called recursively on newly created assignments
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_سونپو_Error);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_سونپو_Error = خطأ;
        m_assignp = nodep;
        إذا (سونپو_Optimize(nodep)) أرجع;
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    سانچا <قسم_ناں T_گرہ_Biop>
    فراغ expandBiOp(T_گرہ_Biop* biopp) {
        Ast_گرہ_Biop* nodep = biopp;
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ثابت Ast_گرہ_DType* ثابت fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_گرہ_Biop* logp = خالی_سوچک;

            ثابت صحيح elements = adtypep->rangep()->elementsConst();
            لكل (صحيح elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_گرہ_Biop* ثابت clonep = جديد T_گرہ_Biop{
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
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            nodep = logp;
        }

        iterateChildren(nodep);
    }

    فراغ ویکھو(AstEq* nodep) override { expandBiOp(nodep); }
    فراغ ویکھو(AstNeq* nodep) override { expandBiOp(nodep); }
    فراغ ویکھو(AstEqCase* nodep) override { expandBiOp(nodep); }
    فراغ ویکھو(AstNeqCase* nodep) override { expandBiOp(nodep); }

    فراغ ویکھو(Ast_ٹکڑا_Sel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_گرہ_اظہاریہ* ثابت fromp = nodep->fromp()->unlink_Fr_پٹھ();
        AstConst* ثابت lop = جديد AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ثابت newp
            = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_ٹکڑا};
        newp->جوڑو_Pinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->did_چوڑائی(سچ);
        newp->protect(خطأ);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ٹکڑا_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ٹکڑا_Visitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::جوڑو_Stat("Optimizations, Slice array skips due to size limit",
                         m_stat_ٹکڑا_Element_Skips);
    }
};

//######################################################################
// Link class functions

فراغ V3_ٹکڑا::ٹکڑا_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ٹکڑا_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("slice", 0, dump_درخت_Either_Level() >= 3);
}
