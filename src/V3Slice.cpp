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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Slice.h"

#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//*************************************************************************

جماعت قاش_Visitor final : عوامی VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNodeAssign::user1()      -> bool.  True if find is complete
    //  AstNodeUniop::user1()       -> bool.  True if find is complete
    //  AstArraySel::user1p()       -> AstVarRef. The VarRef that the final ArraySel points to
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_statAssigns;  // Statistic tracking
    VDouble0 m_stat_قاش_Element_Skips;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ* m_assignp = خالی_اشاریہ;  // Assignment we are under
    منطقی m_تفویض_Error = جھوٹ;  // True if the current assign already has an error
    منطقی m_okConsArray = جھوٹ;  // Allow ConsArray children

    // METHODS
    Ast_گرہ_اظہاریہ* cloneAndSel(Ast_گرہ_اظہاریہ* ثابت nodep, uint64_t elements, uint64_t elemIdx,
                             ثابت منطقی need_خالص) {
        // Insert an ArraySel, except for a few special cases
        ثابت AstUnpackArrayDType* ثابت arrayp
            = VN_CAST(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        اگر (!arrayp) {  // V3Width should have complained, but...
            اگر (!m_تفویض_Error) {
                nodep->v3error(
                    nodep->prettyTypeName()
                    << " is not an unpacked array, but is in an unpacked array context");
            } ورنہ {
                V3Error::incErrors();  // Otherwise might infinite loop
            }
            m_تفویض_Error = سچ;
            // Likely will cause downstream errors
            واپس nodep->clone_درخت(جھوٹ, need_خالص);
        }
        اگر (static_cast<uint64_t>(arrayp->rangep()->elementsConst()) != elements) {
            اگر (!m_تفویض_Error) {
                nodep->v3error(
                    "Slices of arrays in assignments have different unpacked dimensions, "
                    << elements << " versus " << arrayp->rangep()->elementsConst());
            }
            m_تفویض_Error = سچ;
            elements = 1;
            elemIdx = 0;
        }
        Ast_گرہ_اظہاریہ* newp;
        اگر (AstConsArray* ثابت consp = VN_CAST(nodep, ConsArray)) {
            UINFO(9, "  cloneConsArray(" << elements << "," << elemIdx << ") " << nodep);
            ثابت uint64_t اشاریہ
                = arrayp->rangep()->ascending() ? elemIdx : elements - 1 - elemIdx;
            newp = consp->valuep(اشاریہ)->clone_درخت(جھوٹ, need_خالص);
        } ورنہ اگر (ثابت Ast_Cvt_Unpack_مقصد_Unpack* ثابت cvtp = VN_CAST(nodep, Cvt_Unpack_مقصد_Unpack)) {
            UINFO(9, "  cloneCvtUnpackToUnpack(" << elements << "," << elemIdx << ") " << nodep);
            // The element in the same left-to-right position of the source
            Ast_گرہ_اظہاریہ* ثابت elemp = cloneAndSel(cvtp->fromp(), elements, elemIdx, need_خالص);
            Ast_گرہ_DType* ثابت subDTypep = arrayp->subDTypep();
            ثابت AstUnpackArrayDType* ثابت subArrayp
                = VN_CAST(subDTypep->skipRefp(), UnpackArrayDType);
            اگر (!subArrayp || !subArrayp->is_الٹ_کا(elemp->dtypep())) واپس elemp;
            واپس نیا Ast_Cvt_Unpack_مقصد_Unpack{nodep->fileline(), elemp, subDTypep};
        } ورنہ اگر (AstCond* ثابت snodep = VN_CAST(nodep, Cond)) {
            UINFO(9, "  cloneCond(" << elements << "," << elemIdx << ") " << nodep);
            واپس نیا AstCond{snodep->fileline(), snodep->condp()->clone_درخت(جھوٹ, need_خالص),
                               cloneAndSel(snodep->thenp(), elements, elemIdx, need_خالص),
                               cloneAndSel(snodep->elsep(), elements, elemIdx, need_خالص)};
        } ورنہ اگر (ثابت Ast_قاش_Sel* ثابت snodep = VN_CAST(nodep, قاش_Sel)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحیح le_انحراف = (snodep->declRange().lo()
                                  + (!snodep->declRange().ascending()
                                         ? snodep->declRange().elements() - 1 - elemIdx
                                         : elemIdx));
            newp = نیا AstArraySel{nodep->fileline(), snodep->fromp()->clone_درخت(جھوٹ, need_خالص),
                                   le_انحراف};
        } ورنہ اگر (ثابت AstSampled* ثابت snodep = VN_CAST(nodep, Sampled)) {
            UINFO(9, "  cloneSliceSel(" << elements << "," << elemIdx << ") " << nodep);
            Ast_گرہ_اظہاریہ* ثابت exprp = VN_AS(snodep->exprp(), گرہ_اظہاریہ);
            Ast_گرہ_اظہاریہ* ثابت selp = cloneAndSel(exprp, elements, elemIdx, need_خالص);
            واپس نیا AstSampled{nodep->fileline(), selp, selp->dtypep(), snodep->اندرونی_3()};
        } ورنہ اگر (Ast_اظہاریہ_Stmt* ثابت snodep = VN_CAST(nodep, اظہاریہ_Stmt)) {
            UINFO(9, "  cloneExprStmt(" << elements << "," << elemIdx << ") " << nodep);
            Ast_گرہ_اظہاریہ* ثابت نتیجہ_Selp
                = cloneAndSel(snodep->resultp(), elements, elemIdx, need_خالص);
            اگر (snodep->stmtsp()) {
                واپس نیا Ast_اظہاریہ_Stmt{nodep->fileline(), snodep->stmtsp()->unlink_Fr_عقب_With_اگلا(),
                                       نتیجہ_Selp};
            } ورنہ {
                واپس نتیجہ_Selp;
            }
        } ورنہ اگر (VN_IS(nodep, گرہ_Var_Ref) || VN_IS(nodep, گرہ_Sel) || VN_IS(nodep, CMethodHard)
                   || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            UINFO(9, "  cloneSel(" << elements << "," << elemIdx << ") " << nodep);
            ثابت صحیح le_انحراف = !arrayp->rangep()->ascending()
                                     ? arrayp->rangep()->elementsConst() - 1 - elemIdx
                                     : elemIdx;
            newp = نیا AstArraySel{nodep->fileline(), nodep->clone_درخت(جھوٹ, need_خالص), le_انحراف};
        } ورنہ {
            اگر (!m_تفویض_Error) {
                nodep->v3error(nodep->prettyTypeName()
                               << " unexpected in assignment to unpacked array");
            }
            m_تفویض_Error = سچ;
            // Likely will cause downstream errors
            newp = nodep->clone_درخت(جھوٹ, need_خالص);
        }
        واپس newp;
    }

    منطقی تفویض_Optimize(Ast_گرہ_تفویض* nodep) {
        // Return true if did optimization
        Ast_گرہ_DType* ثابت dtp = nodep->lhsp()->dtypep()->skipRefp();
        Ast_گرہ* stp = nodep->rhsp();
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtp, UnpackArrayDType);
        اگر (!arrayp) واپس جھوٹ;
        اگر (VN_IS(stp, Cvt_Packed_مقصد_Array)) واپس جھوٹ;
        اگر (VN_IS(stp, CReset)) واپس جھوٹ;

        // Any isSc variables must be expanded regardless of --fno-slice
        ثابت منطقی hasSc
            = nodep->exists([&](ثابت AstVarRef* refp) -> منطقی { واپس refp->varp()->isSc(); });
        اگر (!hasSc && !v3Global.opt.f_قاش()) {
            m_okConsArray = سچ;  // VL_RESTORER in visit(AstNodeAssign)
            واپس جھوٹ;
        }

        // Skip optimization if array is too large
        ثابت صحیح elements = arrayp->rangep()->elementsConst();
        ثابت صحیح element_حد_بندی = v3Global.opt.f_قاش_Element_حد_بندی();
        اگر (elements > element_حد_بندی && element_حد_بندی > 0) {
            ++m_stat_قاش_Element_Skips;
            m_okConsArray = سچ;  // VL_RESTORER in visit(AstNodeAssign)
            واپس جھوٹ;
        }

        // Skip if this is a simple a = b assignment of identical arrays
        اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            اگر (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                اگر (!hasSc && lhsp->dtypep()->skipRefp()->same_درخت(rhsp->dtypep()->skipRefp())) {
                    m_okConsArray = سچ;  // VL_RESTORER in visit(AstNodeAssign)
                    واپس جھوٹ;
                }
            }
        }

        UINFO(4, "Slice optimizing " << nodep);
        ++m_statAssigns;

        // Left and right could have different ascending/descending range,
        // but #elements is common and all variables are realigned to start at zero
        // Assign of an ascending range slice to a descending range one must reverse
        // the elements
        Ast_گرہ_تفویض* newlistp = خالی_اشاریہ;
        برائے (uint64_t elemIdx = 0; elemIdx < static_cast<uint64_t>(elements); ++elemIdx) {
            // Original node is replaced, so it is safe to copy it one time even if it is impure.
            Ast_گرہ_تفویض* ثابت newp
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
        واپس سچ;
    }

    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        // Called recursively on newly created assignments
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        UINFOTREE(9, nodep, "", "Deslice-In");
        VL_RESTORER(m_تفویض_Error);
        VL_RESTORER(m_assignp);
        VL_RESTORER(m_okConsArray);  // Set in assignOptimize
        m_تفویض_Error = جھوٹ;
        m_assignp = nodep;
        اگر (تفویض_Optimize(nodep)) واپس;
        iterateChildren(nodep);
    }

    خالی دورہ(Ast_گرہ_Cons* nodep) override {
        VL_RESTORER(m_okConsArray);
        m_okConsArray = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstConsArray* nodep) override {
        UASSERT_OBJ(!m_assignp || m_okConsArray, nodep,
                    "Array initialization should have been removed earlier");
    }

    سانچہ <قسم_نام T_گرہ_Biop>
    خالی expandBiOp(T_گرہ_Biop* biopp) {
        Ast_گرہ_Biop* nodep = biopp;
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        UINFO(9, "  Bi-Eq/Neq expansion " << nodep);

        // Only expand if lhs is an unpacked array (we assume type checks already passed)
        ثابت Ast_گرہ_DType* ثابت fromDtp = nodep->lhsp()->dtypep()->skipRefp();
        اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            Ast_گرہ_Biop* logp = خالی_اشاریہ;

            ثابت صحیح elements = adtypep->rangep()->elementsConst();
            برائے (صحیح elemIdx = 0; elemIdx < elements; ++elemIdx) {
                // EQ(a,b) -> LOGAND(EQ(ARRAYSEL(a,0), ARRAYSEL(b,0)), ...[1])
                // Original node is replaced, so it is safe to copy it one time even if it is
                // impure.
                T_گرہ_Biop* ثابت clonep = نیا T_گرہ_Biop{
                    nodep->fileline(), cloneAndSel(nodep->lhsp(), elements, elemIdx, elemIdx != 0),
                    cloneAndSel(nodep->rhsp(), elements, elemIdx, elemIdx != 0)};
                اگر (!logp) {
                    logp = clonep;
                } ورنہ {
                    انتخاب (nodep->type()) {
                    حالت VNType::Eq:  // FALLTHRU
                    حالت VNType::EqCase:
                        logp = نیا AstLogAnd{nodep->fileline(), logp, clonep};
                        توڑو;
                    حالت VNType::Neq:  // FALLTHRU
                    حالت VNType::NeqCase:
                        logp = نیا AstLogOr{nodep->fileline(), logp, clonep};
                        توڑو;
                    طےشدہ: nodep->v3fatalSrc("Unknown node type processing array slice"); توڑو;
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

    خالی دورہ(AstEq* nodep) override { expandBiOp(nodep); }
    خالی دورہ(AstNeq* nodep) override { expandBiOp(nodep); }
    خالی دورہ(AstEqCase* nodep) override { expandBiOp(nodep); }
    خالی دورہ(AstNeqCase* nodep) override { expandBiOp(nodep); }

    خالی دورہ(Ast_قاش_Sel* nodep) override {
        // Slice used as a bare value, e.g. a $display argument. Build it via
        // VlUnpacked::slice<N_Out>(loIdx), as DYN_SLICE already does for Queues.
        iterateChildren(nodep);
        Ast_گرہ_اظہاریہ* ثابت fromp = nodep->fromp()->unlink_Fr_عقب();
        AstConst* ثابت lop = نیا AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, 32,
                                           static_cast<uint32_t>(nodep->declRange().lo())};
        AstCMethodHard* ثابت newp
            = نیا AstCMethodHard{nodep->fileline(), fromp, VCMethod::ARRAY_قاش};
        newp->جوڑو_Pinsp(lop);
        newp->dtypeFrom(nodep);  // Reuse the already-correct sliced array dtype
        newp->did_چوڑائی(سچ);
        newp->protect(جھوٹ);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح قاش_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~قاش_Visitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Slice array assignments", m_statAssigns);
        V3Stats::جوڑو_Stat("Optimizations, Slice array skips due to size limit",
                         m_stat_قاش_Element_Skips);
    }
};

//######################################################################
// Link class functions

خالی V3_قاش::قاش_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { قاش_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("slice", 0, dump_درخت_Either_Level() >= 3);
}
