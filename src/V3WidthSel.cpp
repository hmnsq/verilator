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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Const.h"
#شامل "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

رده WidthSelVisitor final : عمومی VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    تهی checkConstantOrReplace(Ast_گره* nodep, ثابت string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        اگر (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    ساختار FromData final {
        Ast_گره_DType* ثابت m_errp;  // Node that was found, for error reporting if not known type
        Ast_گره_DType* ثابت m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_گره_DType* errp, Ast_گره_DType* dtypep, ثابت VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = پیشفرض;
    };
    ایستا FromData fromDataForArray(Ast_گره* nodep, Ast_گره* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        وقتی (basefromp) {
            اگر (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                ادامه;
            }
            بشکن;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_گره_DType* ثابت ddtypep = basefromp->dtypep()->skipRefp();
        Ast_گره_DType* ثابت errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        اگر (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(ddtypep, گره_Array_DType)) {
            fromRange = adtypep->declRange();
        } وگرنه اگر (VN_IS(ddtypep, AssocArrayDType)) {
        } وگرنه اگر (VN_IS(ddtypep, WildcardArrayDType)) {
        } وگرنه اگر (VN_IS(ddtypep, DynArrayDType)) {
        } وگرنه اگر (VN_IS(ddtypep, QueueDType)) {
        } وگرنه اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت adtypep
                   = VN_CAST(ddtypep, گره_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } وگرنه اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(ddtypep, BasicDType)) {
            اگر (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } وگرنه اگر (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } وگرنه {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } وگرنه {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        بازگشت FromData(errp, ddtypep, fromRange);
    }

    Ast_گره_عبارت* newSubNeg(Ast_گره_عبارت* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        اگر (rhs == 0) {
            بازگشت lhsp;
        } وگرنه اگر (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_عدد رقم(lhsp, lhsp->width());
            رقم.opSub(VN_AS(lhsp, Const)->رقم(), V3_عدد(lhsp, 32, rhs));
            رقم.isSigned(lhsp->isSigned());
            بازگشت جدید AstConst{lhsp->fileline(), رقم};
        } وگرنه اگر (rhs > 0) {
            Ast_گره_عبارت* ثابت newp
                = جدید AstSub{lhsp->fileline(), lhsp,
                             جدید AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            بازگشت newp;
        } وگرنه {  // rhs < 0;
            Ast_گره_عبارت* ثابت newp
                = جدید AstAdd{lhsp->fileline(), lhsp,
                             جدید AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            بازگشت newp;
        }
    }
    Ast_گره_عبارت* newSubNeg(int32_t lhs, Ast_گره_عبارت* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_گره_عبارت* ثابت newp = جدید AstSub{
            rhsp->fileline(), جدید AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        بازگشت newp;
    }

    Ast_گره_عبارت* newSubLsbOf(Ast_گره_عبارت* underp, ثابت VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        اگر (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            بازگشت underp;
        } وگرنه {
            اگر (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                بازگشت newSubNeg(fromRange.hi(), underp);
            } وگرنه {
                // reg [3:1] needs a SUB(under,1)
                بازگشت newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_گره_عبارت* newMulConst(File_سطر* fl, uint32_t elwidth, Ast_گره_عبارت* indexp) {
        Ast_گره_عبارت* extendp;
        ثابت صحیح widthMin = std::max(V3_عدد::log2b(elwidth) + 1, indexp->widthMin());
        اگر (indexp->width() > 32) {
            extendp = جدید AstSel{fl, indexp, 0, 32, widthMin};
        } وگرنه {
            extendp = جدید AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_گره_عبارت* ثابت mulp
            = جدید AstMul{fl, جدید AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        بازگشت mulp;
    }

    Ast_گره_DType* sliceDType(AstPackArrayDType* nodep, صحیح msb, صحیح lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        اگر (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            بازگشت nodep;
        } وگرنه {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ثابت VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_گره_DType* ثابت vardtypep
                = جدید AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        جدید AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            بازگشت vardtypep;
        }
    }

    Ast_گره_عبارت* selQueueBackness(Ast_گره* nodep) {
        اگر (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            بازگشت جدید AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } وگرنه اگر (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_گره_عبارت* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            بازگشت subrhsp;
        } وگرنه {
            بازگشت nullptr;
        }
    }

    تهی warnTri(Ast_گره* nodep) {
        اگر (VN_IS(nodep, Const) && VN_AS(nodep, Const)->رقم().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    تهی visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ثابت FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_گره_DType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_گره_عبارت* subp = rhsp;
            اگر (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ثابت newp = جدید AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_گره_عبارت* subp = rhsp;
            اگر (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } وگرنه {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ثابت صحیح elwidth = adtypep->width() / fromRange.elements();
            AstSel* ثابت newp = جدید AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            اگر (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(نادرست));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ثابت newp = جدید AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (ثابت AstWildcardArrayDType* ثابت adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ثابت newp = جدید AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ثابت VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ثابت newp
                = جدید AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (ثابت AstQueueDType* ثابت adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            اگر (Ast_گره_عبارت* ثابت backnessp = selQueueBackness(rhsp)) {
                ثابت VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = جدید AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } وگرنه {
                ثابت VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = جدید AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_گره_عبارت* exprp = fromp;
            وقتی (درست) {
                اگر (AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } وگرنه اگر (AstStructSel* ثابت structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } وگرنه {
                    بشکن;
                }
            }
            ثابت Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(exprp, گره_Var_Ref);
            Ast_گره_عبارت* newp;
            اگر (!varrefp || varrefp->access().isReadOnly()) {
                newp = جدید AstGetcN{nodep->fileline(), fromp, rhsp};
            } وگرنه {
                newp = جدید AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ثابت newp
                = جدید AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (VN_IS(ddtypep, گره_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ثابت newp
                = جدید AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        اگر (!rhsp->backp()) VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
    }
    تهی visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlinkFrBack();
        ثابت FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_گره_DType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        اگر (VN_IS(ddtypep, QueueDType)) {
            Ast_گره_عبارت* ثابت qleftp = nodep->leftp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت qrightp = nodep->rightp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت qleftBacknessp = selQueueBackness(qleftp);
            Ast_گره_عبارت* ثابت qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ثابت VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* ثابت newp = جدید AstCMethodHard{nodep->fileline(), fromp, method};
            اگر (qleftBacknessp) {
                VL_DO_DANGLING(افزودن_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } وگرنه {
                newp->addPinsp(qleftp);
            }
            اگر (qrightBacknessp) {
                VL_DO_DANGLING(افزودن_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } وگرنه {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(درست);
            newp->protect(نادرست);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_گره_عبارت* ثابت msbp = nodep->leftp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        ثابت int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        اگر (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            اگر (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه اگر (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ثابت newp = جدید AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه {  // Slice
                AstSliceSel* ثابت newp
                    = جدید AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        } وگرنه اگر (AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            اگر (fromRange.ascending()) std::swap(msb, lsb);
            اگر (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            ثابت صحیح elwidth = adtypep->width() / fromRange.elements();
            AstSel* ثابت newp
                = جدید AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            اگر (fromRange.ascending()) std::swap(msb, lsb);
            اگر (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ثابت newp = جدید AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه اگر (VN_IS(ddtypep, گره_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            اگر (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ثابت newp = جدید AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        اگر (!fromp->backp()) VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
        اگر (!msbp->backp()) VL_DO_DANGLING(افزودن_Deletep(msbp), msbp);
        اگر (!lsbp->backp()) VL_DO_DANGLING(افزودن_Deletep(lsbp), lsbp);
    }

    تهی replaceSelPlusMinus(Ast_گره_Pre_Sel* nodep) {
        // Select of a range specified with +: or -:, i.e. "array[2+:3], [2-:3]"
        // This select style has a lsb and width
        UINFO(6, "SELPLUS/MINUS " << nodep);
        // Below 2 lines may change nodep->widthp()
        UINFOTREE(9, nodep, "", "SELPM0");
        V3Width::widthParamsEdit(nodep->rhsp());  // constifyEdit doesn't ensure widths finished
        V3Const::constifyEdit(nodep->rhsp());  // May relink pointed to node, ok if not const
        V3Const::constifyParamsEdit(nodep->thsp());  // May relink pointed to node
        checkConstantOrReplace(nodep->thsp(),
                               "Width of :+ or :- bit slice range isn't a constant");
        UINFOTREE(9, nodep, "", "SELPM3");
        // Now replace it with an AstSel
        Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        ثابت صحیح width = VN_AS(widthp, Const)->toSInt();
        اگر (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        اگر (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ثابت FromData fromdata = fromDataForArray(nodep, fromp);
        ثابت Ast_گره_DType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        اگر (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            اگر (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه اگر (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ثابت newp = جدید AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه اگر (VN_IS(rhsp, Const)) {  // Slice
                ثابت int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ثابت int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                ثابت int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* ثابت newp = جدید AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وگرنه {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } وگرنه اگر (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, گره_UOr_Struct_DType)
                       && VN_AS(ddtypep, گره_UOr_Struct_DType)->packedUnsup())) {
            صحیح elwidth = 1;
            صحیح newwidth = width;
            اگر (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_گره_عبارت* newlsbp = nullptr;
            اگر (VN_IS(nodep, SelPlus)) {
                اگر (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } وگرنه {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } وگرنه اگر (VN_IS(nodep, SelMinus)) {
                اگر (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } وگرنه {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } وگرنه {
                nodep->v3fatalSrc("Bad Case");
            }
            اگر (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ثابت newp = جدید AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        اگر (!fromp->backp()) VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
        اگر (!rhsp->backp()) VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
        اگر (!widthp->backp()) VL_DO_DANGLING(افزودن_Deletep(widthp), widthp);
    }
    تهی visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    تهی visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    تهی visit(Ast_گره* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

عمومی:
    // CONSTRUCTORS
    WidthSelVisitor() = پیشفرض;
    ~WidthSelVisitor() override = پیشفرض;
    Ast_گره* mainAcceptEdit(Ast_گره* nodep) { بازگشت iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_گره* V3Width::widthSelNoIterEdit(Ast_گره* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    بازگشت nodep;
}

#undef iterateChildren
