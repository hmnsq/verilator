// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Expression width calculations
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
// V3WidthSel's Transformations:
//      Top down traversal:
//          Replace SELPLUS/SELMINUS with SEL
//          Replace SELEXTRACT with SEL
//          Replace SELBIT with SEL or ARRAYSEL
//
// This code was once in V3LinkResolve, but ascending bit range vectors won't
// work that early.  It was considered for V3Width and V3Param, but is
// fairly ugly both places as the nodes change in too strongly
// interconnected ways.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Const.h"
#ضمن "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

صنف عرض_Sel_Visitor final : عام VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#تعریف_کن iterateChildren DO_NOT_iterate_Childern_درون_V3_عرض_Sel

    // METHODS

    فراغ checkConstantOrReplace(Ast_گره* nodep, ثابت string& پیام) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        إذا (!VN_IS(nodep, Const)) {
            nodep->v3error(پیام);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    بنية From_داده final {
        Ast_گره_DType* ثابت m_errp;  // Node that was found, for error reporting if not known type
        Ast_گره_DType* ثابت m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        From_داده(Ast_گره_DType* errp, Ast_گره_DType* dtypep, ثابت VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~From_داده() = افتراضي;
    };
    ساكن From_داده from_داده_For_Array(Ast_گره* nodep, Ast_گره* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        طالما (basefromp) {
            إذا (VN_IS(basefromp, Attr_از)) {
                basefromp = VN_AS(basefromp, Attr_از)->fromp();
                استمر;
            }
            اكسر;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_گره_DType* ثابت ddtypep = basefromp->dtypep()->skipRefp();
        Ast_گره_DType* ثابت errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        إذا (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(ddtypep, گره_Array_DType)) {
            fromRange = adtypep->declRange();
        } وإلا إذا (VN_IS(ddtypep, AssocArrayDType)) {
        } وإلا إذا (VN_IS(ddtypep, WildcardArrayDType)) {
        } وإلا إذا (VN_IS(ddtypep, DynArrayDType)) {
        } وإلا إذا (VN_IS(ddtypep, صف_DType)) {
        } وإلا إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت adtypep
                   = VN_CAST(ddtypep, گره_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(ddtypep, BasicDType)) {
            إذا (adtypep->isString() && VN_IS(nodep, Sel_بیت)) {
            } وإلا إذا (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } وإلا {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } وإلا {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        أرجع From_داده(errp, ddtypep, fromRange);
    }

    Ast_گره_عبارت* newSubNeg(Ast_گره_عبارت* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        إذا (rhs == 0) {
            أرجع lhsp;
        } وإلا إذا (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_عدد رقم(lhsp, lhsp->عرض());
            رقم.opSub(VN_AS(lhsp, Const)->رقم(), V3_عدد(lhsp, 32, rhs));
            رقم.isSigned(lhsp->isSigned());
            أرجع جديد AstConst{lhsp->fileline(), رقم};
        } وإلا إذا (rhs > 0) {
            Ast_گره_عبارت* ثابت newp
                = جديد AstSub{lhsp->fileline(), lhsp,
                             جديد AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            أرجع newp;
        } وإلا {  // rhs < 0;
            Ast_گره_عبارت* ثابت newp
                = جديد Ast_جمع_کن{lhsp->fileline(), lhsp,
                             جديد AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            أرجع newp;
        }
    }
    Ast_گره_عبارت* newSubNeg(int32_t lhs, Ast_گره_عبارت* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_گره_عبارت* ثابت newp = جديد AstSub{
            rhsp->fileline(), جديد AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        أرجع newp;
    }

    Ast_گره_عبارت* new_Sub_Lsb_از(Ast_گره_عبارت* underp, ثابت VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        إذا (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            أرجع underp;
        } وإلا {
            إذا (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                أرجع newSubNeg(fromRange.hi(), underp);
            } وإلا {
                // reg [3:1] needs a SUB(under,1)
                أرجع newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_گره_عبارت* newMulConst(File_سطر* fl, uint32_t elwidth, Ast_گره_عبارت* indexp) {
        Ast_گره_عبارت* extendp;
        ثابت صحيح عرض_Min = std::max(V3_عدد::log2b(elwidth) + 1, indexp->عرض_Min());
        إذا (indexp->عرض() > 32) {
            extendp = جديد AstSel{fl, indexp, 0, 32, عرض_Min};
        } وإلا {
            extendp = جديد AstExtend{fl, indexp, 32, عرض_Min};
        }
        Ast_گره_عبارت* ثابت mulp
            = جديد AstMul{fl, جديد AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        أرجع mulp;
    }

    Ast_گره_DType* برش_DType(AstPackArrayDType* nodep, صحيح msb, صحيح lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        إذا (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            أرجع nodep;
        } وإلا {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ثابت VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_گره_DType* ثابت vardtypep
                = جديد AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        جديد AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(vardtypep);
            أرجع vardtypep;
        }
    }

    Ast_گره_عبارت* sel_صف_Backness(Ast_گره* nodep) {
        إذا (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            أرجع جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } وإلا إذا (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_گره_عبارت* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlink_Fr_پشت();
            أرجع subrhsp;
        } وإلا {
            أرجع اشاره_تهی;
        }
    }

    فراغ warnTri(Ast_گره* nodep) {
        إذا (VN_IS(nodep, Const) && VN_AS(nodep, Const)->رقم().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    فراغ بازدید(Ast_Sel_بیت* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت rhsp = nodep->bitp()->unlink_Fr_پشت();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ثابت From_داده fromdata = from_داده_For_Array(nodep, fromp);
        Ast_گره_DType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_گره_عبارت* subp = rhsp;
            إذا (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ثابت newp = جديد AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_گره_عبارت* subp = rhsp;
            إذا (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } وإلا {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->عرض() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->عرض() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ثابت صحيح elwidth = adtypep->عرض() / fromRange.elements();
            AstSel* ثابت newp = جديد AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->decl_El_عرض(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            إذا (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(خطأ));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ثابت newp = جديد AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (ثابت AstWildcardArrayDType* ثابت adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ثابت newp = جديد AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ثابت VCMethod روش
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ثابت newp
                = جديد AstCMethodHard{nodep->fileline(), fromp, روش, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (ثابت Ast_صف_DType* ثابت adtypep = VN_CAST(ddtypep, صف_DType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            إذا (Ast_گره_عبارت* ثابت backnessp = sel_صف_Backness(rhsp)) {
                ثابت VCMethod روش = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_پشت
                                            : VCMethod::ARRAY_AT_پشت;
                newp = جديد AstCMethodHard{nodep->fileline(), fromp, روش, backnessp};
            } وإلا {
                ثابت VCMethod روش = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = جديد AstCMethodHard{nodep->fileline(), fromp, روش, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_گره_عبارت* exprp = fromp;
            طالما (درست) {
                إذا (AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } وإلا إذا (AstStructSel* ثابت structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } وإلا {
                    اكسر;
                }
            }
            ثابت Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(exprp, گره_Var_Ref);
            Ast_گره_عبارت* newp;
            إذا (!varrefp || varrefp->access().isReadOnly()) {
                newp = جديد AstGetcN{nodep->fileline(), fromp, rhsp};
            } وإلا {
                newp = جديد AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), fromp, new_Sub_Lsb_از(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (VN_IS(ddtypep, گره_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), fromp, new_Sub_Lsb_از(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        إذا (!rhsp->backp()) VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
    }
    فراغ بازدید(Ast_Sel_استخراج* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlink_Fr_پشت();
        ثابت From_داده fromdata = from_داده_For_Array(nodep, fromp);
        Ast_گره_DType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        إذا (VN_IS(ddtypep, صف_DType)) {
            Ast_گره_عبارت* ثابت qleftp = nodep->leftp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت qrightp = nodep->rightp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت qleftBacknessp = sel_صف_Backness(qleftp);
            Ast_گره_عبارت* ثابت qrightBacknessp = sel_صف_Backness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ثابت VCMethod روش = (qleftBacknessp    ? VCMethod::DYN_برش_پشت_پشت
                                     : qrightBacknessp ? VCMethod::DYN_برش_FRONT_پشت
                                                       : VCMethod::DYN_برش);
            AstCMethodHard* ثابت newp = جديد AstCMethodHard{nodep->fileline(), fromp, روش};
            إذا (qleftBacknessp) {
                VL_DO_DANGLING(افزودن_Deletep(qleftp), qleftp);
                newp->جمع_کن_Pinsp(qleftBacknessp);
            } وإلا {
                newp->جمع_کن_Pinsp(qleftp);
            }
            إذا (qrightBacknessp) {
                VL_DO_DANGLING(افزودن_Deletep(qrightp), qrightp);
                newp->جمع_کن_Pinsp(qrightBacknessp);
            } وإلا {
                newp->جمع_کن_Pinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->did_عرض(درست);
            newp->protect(خطأ);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_گره_عبارت* ثابت msbp = nodep->leftp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت lsbp = nodep->rightp()->unlink_Fr_پشت();
        int32_t msb = VN_AS(msbp, Const)->مقصد_SInt();
        int32_t lsb = VN_AS(lsbp, Const)->مقصد_SInt();
        ثابت int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        إذا (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            إذا (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا إذا (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ثابت newp = جديد AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا {  //برش
                Ast_برش_Sel* ثابت newp
                    = جديد Ast_برش_Sel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        } وإلا إذا (AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->عرض() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->عرض() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            إذا (fromRange.ascending()) std::جابجا(msb, lsb);
            إذا (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::جابجا(msb, lsb);
            }
            ثابت صحيح elwidth = adtypep->عرض() / fromRange.elements();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, new_Sub_Lsb_از(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->decl_El_عرض(elwidth);
            newp->dtypeFrom(برش_DType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->عرض_Min() == newp->عرض_Const(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            إذا (fromRange.ascending()) std::جابجا(msb, lsb);
            إذا (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::جابجا(msb, lsb);
            }
            AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, new_Sub_Lsb_از(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (VN_IS(ddtypep, گره_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            إذا (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::جابجا(msb, lsb);
            }
            AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, new_Sub_Lsb_از(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        إذا (!fromp->backp()) VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
        إذا (!msbp->backp()) VL_DO_DANGLING(افزودن_Deletep(msbp), msbp);
        إذا (!lsbp->backp()) VL_DO_DANGLING(افزودن_Deletep(lsbp), lsbp);
    }

    فراغ replaceSelPlusMinus(Ast_گره_Pre_Sel* nodep) {
        // Select of a range specified with +: or -:, i.e. "array[2+:3], [2-:3]"
        // This select style has a lsb and width
        UINFO(6, "SELPLUS/MINUS " << nodep);
        // Below 2 lines may change nodep->widthp()
        UINFOTREE(9, nodep, "", "SELPM0");
        V3_عرض::عرض_Params_Edit(nodep->rhsp());  // constifyEdit doesn't ensure widths finished
        V3Const::constifyEdit(nodep->rhsp());  // May relink pointed to node, ok if not const
        V3Const::constifyParamsEdit(nodep->thsp());  // May relink pointed to node
        checkConstantOrReplace(nodep->thsp(),
                               "Width of :+ or :- bit slice range isn't a constant");
        UINFOTREE(9, nodep, "", "SELPM3");
        // Now replace it with an AstSel
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت widthp = nodep->thsp()->unlink_Fr_پشت();
        warnTri(rhsp);
        ثابت صحيح عرض = VN_AS(widthp, Const)->مقصد_SInt();
        إذا (عرض > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        إذا (عرض < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ثابت From_داده fromdata = from_داده_For_Array(nodep, fromp);
        ثابت Ast_گره_DType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        إذا (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            إذا (fromRange.elements() == عرض && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->مقصد_SInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا إذا (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ثابت newp = جديد AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا إذا (VN_IS(rhsp, Const)) {  //برش
                ثابت int32_t rhs = VN_AS(rhsp, Const)->مقصد_SInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ثابت int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + عرض - 1 : rhs) - fromRange.lo();
                ثابت int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - عرض + 1) - fromRange.lo();
                Ast_برش_Sel* ثابت newp = جديد Ast_برش_Sel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } وإلا إذا (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, گره_UOr_Struct_DType)
                       && VN_AS(ddtypep, گره_UOr_Struct_DType)->packedUnsup())) {
            صحيح elwidth = 1;
            صحيح newwidth = عرض;
            إذا (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->عرض() / fromRange.elements();
                newwidth = عرض * elwidth;
            }
            Ast_گره_عبارت* newlsbp = اشاره_تهی;
            إذا (VN_IS(nodep, SelPlus)) {
                إذا (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - عرض + 1), rhsp);
                } وإلا {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } وإلا إذا (VN_IS(nodep, SelMinus)) {
                إذا (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } وإلا {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (عرض - 1));
                }
            } وإلا {
                nodep->v3fatalSrc("Bad Case");
            }
            إذا (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->decl_El_عرض(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        إذا (!fromp->backp()) VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
        إذا (!rhsp->backp()) VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
        إذا (!widthp->backp()) VL_DO_DANGLING(افزودن_Deletep(widthp), widthp);
    }
    فراغ بازدید(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    فراغ بازدید(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    فراغ بازدید(Ast_گره* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

عام:
    // CONSTRUCTORS
    عرض_Sel_Visitor() = افتراضي;
    ~عرض_Sel_Visitor() override = افتراضي;
    Ast_گره* mainAcceptEdit(Ast_گره* nodep) { أرجع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_گره* V3_عرض::عرض_Sel_No_Iter_Edit(Ast_گره* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    عرض_Sel_Visitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

#لغو_تعریف iterateChildren
