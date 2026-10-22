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

صنف WidthSelVisitor final : عام VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    فراغ checkConstantOrReplace(AstNode* nodep, ثابت string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        𐤀𐤌 (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    بنية FromData final {
        AstNodeDType* ثابت m_errp;  // Node that was found, for error reporting if not known type
        AstNodeDType* ثابت m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(AstNodeDType* errp, AstNodeDType* dtypep, ثابت VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = افتراضي;
    };
    ساكن FromData fromDataForArray(AstNode* nodep, AstNode* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        طالما (basefromp) {
            𐤀𐤌 (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                استمر;
            }
            اكسر;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        AstNodeDType* ثابت ddtypep = basefromp->dtypep()->skipRefp();
        AstNodeDType* ثابت errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        𐤀𐤌 (ثابت AstNodeArrayDType* ثابت adtypep = VN_CAST(ddtypep, NodeArrayDType)) {
            fromRange = adtypep->declRange();
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, AssocArrayDType)) {
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, WildcardArrayDType)) {
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, DynArrayDType)) {
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, QueueDType)) {
        } وإلا 𐤀𐤌 (ثابت AstNodeUOrStructDType* ثابت adtypep
                   = VN_CAST(ddtypep, NodeUOrStructDType)) {
            fromRange = adtypep->declRange();
        } وإلا 𐤀𐤌 (ثابت AstBasicDType* ثابت adtypep = VN_CAST(ddtypep, BasicDType)) {
            𐤀𐤌 (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } وإلا 𐤀𐤌 (adtypep->isRanged()) {
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
        أرجع FromData(errp, ddtypep, fromRange);
    }

    AstNodeExpr* newSubNeg(AstNodeExpr* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        𐤀𐤌 (rhs == 0) {
            أرجع lhsp;
        } وإلا 𐤀𐤌 (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3Number num(lhsp, lhsp->width());
            num.opSub(VN_AS(lhsp, Const)->num(), V3Number(lhsp, 32, rhs));
            num.isSigned(lhsp->isSigned());
            أرجع جديد AstConst{lhsp->fileline(), num};
        } وإلا 𐤀𐤌 (rhs > 0) {
            AstNodeExpr* ثابت newp
                = جديد AstSub{lhsp->fileline(), lhsp,
                             جديد AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            أرجع newp;
        } وإلا {  // rhs < 0;
            AstNodeExpr* ثابت newp
                = جديد AstAdd{lhsp->fileline(), lhsp,
                             جديد AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            أرجع newp;
        }
    }
    AstNodeExpr* newSubNeg(int32_t lhs, AstNodeExpr* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        AstNodeExpr* ثابت newp = جديد AstSub{
            rhsp->fileline(), جديد AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        أرجع newp;
    }

    AstNodeExpr* newSubLsbOf(AstNodeExpr* underp, ثابت VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        𐤀𐤌 (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            أرجع underp;
        } وإلا {
            𐤀𐤌 (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                أرجع newSubNeg(fromRange.hi(), underp);
            } وإلا {
                // reg [3:1] needs a SUB(under,1)
                أرجع newSubNeg(underp, fromRange.lo());
            }
        }
    }
    AstNodeExpr* newMulConst(FileLine* fl, uint32_t elwidth, AstNodeExpr* indexp) {
        AstNodeExpr* extendp;
        ثابت صحيح widthMin = std::max(V3Number::log2b(elwidth) + 1, indexp->widthMin());
        𐤀𐤌 (indexp->width() > 32) {
            extendp = جديد AstSel{fl, indexp, 0, 32, widthMin};
        } وإلا {
            extendp = جديد AstExtend{fl, indexp, 32, widthMin};
        }
        AstNodeExpr* ثابت mulp
            = جديد AstMul{fl, جديد AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        أرجع mulp;
    }

    AstNodeDType* sliceDType(AstPackArrayDType* nodep, صحيح msb, صحيح lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        𐤀𐤌 (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            أرجع nodep;
        } وإلا {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ثابت VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            AstNodeDType* ثابت vardtypep
                = جديد AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        جديد AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            أرجع vardtypep;
        }
    }

    AstNodeExpr* selQueueBackness(AstNode* nodep) {
        𐤀𐤌 (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            أرجع جديد AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } وإلا 𐤀𐤌 (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            AstNodeExpr* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            أرجع subrhsp;
        } وإلا {
            أرجع nullptr;
        }
    }

    فراغ warnTri(AstNode* nodep) {
        𐤀𐤌 (VN_IS(nodep, Const) && VN_AS(nodep, Const)->num().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    فراغ visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        AstNodeExpr* ثابت fromp = nodep->fromp()->unlinkFrBack();
        AstNodeExpr* ثابت rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ثابت FromData fromdata = fromDataForArray(nodep, fromp);
        AstNodeDType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        𐤀𐤌 (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            AstNodeExpr* subp = rhsp;
            𐤀𐤌 (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ثابت newp = جديد AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            AstNodeExpr* subp = rhsp;
            𐤀𐤌 (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } وإلا {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ثابت صحيح elwidth = adtypep->width() / fromRange.elements();
            AstSel* ثابت newp = جديد AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            𐤀𐤌 (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(خطأ));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ثابت newp = جديد AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (ثابت AstWildcardArrayDType* ثابت adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ثابت newp = جديد AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (ثابت AstDynArrayDType* ثابت adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ثابت VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ثابت newp
                = جديد AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (ثابت AstQueueDType* ثابت adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            𐤀𐤌 (AstNodeExpr* ثابت backnessp = selQueueBackness(rhsp)) {
                ثابت VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = جديد AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } وإلا {
                ثابت VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = جديد AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            AstNodeExpr* exprp = fromp;
            طالما (true) {
                𐤀𐤌 (AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } وإلا 𐤀𐤌 (AstStructSel* ثابت structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } وإلا {
                    اكسر;
                }
            }
            ثابت AstNodeVarRef* ثابت varrefp = VN_CAST(exprp, NodeVarRef);
            AstNodeExpr* newp;
            𐤀𐤌 (!varrefp || varrefp->access().isReadOnly()) {
                newp = جديد AstGetcN{nodep->fileline(), fromp, rhsp};
            } وإلا {
                newp = جديد AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, NodeUOrStructDType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
        𐤀𐤌 (!rhsp->backp()) VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
    }
    فراغ visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        AstNodeExpr* ثابت fromp = nodep->fromp()->unlinkFrBack();
        ثابت FromData fromdata = fromDataForArray(nodep, fromp);
        AstNodeDType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        𐤀𐤌 (VN_IS(ddtypep, QueueDType)) {
            AstNodeExpr* ثابت qleftp = nodep->leftp()->unlinkFrBack();
            AstNodeExpr* ثابت qrightp = nodep->rightp()->unlinkFrBack();
            AstNodeExpr* ثابت qleftBacknessp = selQueueBackness(qleftp);
            AstNodeExpr* ثابت qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ثابت VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* ثابت newp = جديد AstCMethodHard{nodep->fileline(), fromp, method};
            𐤀𐤌 (qleftBacknessp) {
                VL_DO_DANGLING(pushDeletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } وإلا {
                newp->addPinsp(qleftp);
            }
            𐤀𐤌 (qrightBacknessp) {
                VL_DO_DANGLING(pushDeletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } وإلا {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(true);
            newp->protect(خطأ);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        AstNodeExpr* ثابت msbp = nodep->leftp()->unlinkFrBack();
        AstNodeExpr* ثابت lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        ثابت int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        𐤀𐤌 (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            𐤀𐤌 (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } وإلا 𐤀𐤌 (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ثابت newp = جديد AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } وإلا {  // Slice
                AstSliceSel* ثابت newp
                    = جديد AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        } وإلا 𐤀𐤌 (AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            𐤀𐤌 (fromRange.ascending()) std::swap(msb, lsb);
            𐤀𐤌 (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            ثابت صحيح elwidth = adtypep->width() / fromRange.elements();
            AstSel* ثابت newp
                = جديد AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            𐤀𐤌 (fromRange.ascending()) std::swap(msb, lsb);
            𐤀𐤌 (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, NodeUOrStructDType)) {
            // Classes don't have an ascending range
            𐤀𐤌 (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        𐤀𐤌 (!fromp->backp()) VL_DO_DANGLING(pushDeletep(fromp), fromp);
        𐤀𐤌 (!msbp->backp()) VL_DO_DANGLING(pushDeletep(msbp), msbp);
        𐤀𐤌 (!lsbp->backp()) VL_DO_DANGLING(pushDeletep(lsbp), lsbp);
    }

    فراغ replaceSelPlusMinus(AstNodePreSel* nodep) {
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
        AstNodeExpr* ثابت fromp = nodep->fromp()->unlinkFrBack();
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* ثابت widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        ثابت صحيح width = VN_AS(widthp, Const)->toSInt();
        𐤀𐤌 (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        𐤀𐤌 (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ثابت FromData fromdata = fromDataForArray(nodep, fromp);
        ثابت AstNodeDType* ثابت ddtypep = fromdata.m_dtypep;
        ثابت VNumRange fromRange = fromdata.m_fromRange;
        𐤀𐤌 (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            𐤀𐤌 (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } وإلا 𐤀𐤌 (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ثابت newp = جديد AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } وإلا 𐤀𐤌 (VN_IS(rhsp, Const)) {  // Slice
                ثابت int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ثابت int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                ثابت int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* ثابت newp = جديد AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } وإلا {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } وإلا 𐤀𐤌 (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, NodeUOrStructDType)
                       && VN_AS(ddtypep, NodeUOrStructDType)->packedUnsup())) {
            صحيح elwidth = 1;
            صحيح newwidth = width;
            𐤀𐤌 (ثابت AstPackArrayDType* ثابت adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            AstNodeExpr* newlsbp = nullptr;
            𐤀𐤌 (VN_IS(nodep, SelPlus)) {
                𐤀𐤌 (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } وإلا {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } وإلا 𐤀𐤌 (VN_IS(nodep, SelMinus)) {
                𐤀𐤌 (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } وإلا {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } وإلا {
                nodep->v3fatalSrc("Bad Case");
            }
            𐤀𐤌 (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ثابت newp = جديد AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        𐤀𐤌 (!fromp->backp()) VL_DO_DANGLING(pushDeletep(fromp), fromp);
        𐤀𐤌 (!rhsp->backp()) VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
        𐤀𐤌 (!widthp->backp()) VL_DO_DANGLING(pushDeletep(widthp), widthp);
    }
    فراغ visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    فراغ visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    فراغ visit(AstNode* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

عام:
    // CONSTRUCTORS
    WidthSelVisitor() = افتراضي;
    ~WidthSelVisitor() override = افتراضي;
    AstNode* mainAcceptEdit(AstNode* nodep) { أرجع iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

AstNode* V3Width::widthSelNoIterEdit(AstNode* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    أرجع nodep;
}

#undef iterateChildren
