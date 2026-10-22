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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

वर्ग WidthSelVisitor final : सार्वजनिक VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    शून्य checkConstantOrReplace(Ast_गाठ* nodep, ध्रुव string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        यदि (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    संरचना FromData final {
        Ast_गाठ_DType* ध्रुव m_errp;  // Node that was found, for error reporting if not known type
        Ast_गाठ_DType* ध्रुव m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_गाठ_DType* errp, Ast_गाठ_DType* dtypep, ध्रुव VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = स्वभाव;
    };
    स्थिर FromData fromDataForArray(Ast_गाठ* nodep, Ast_गाठ* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        यावत् (basefromp) {
            यदि (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                अनुवर्त;
            }
            भङ्ग;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_गाठ_DType* ध्रुव ddtypep = basefromp->dtypep()->skipRefp();
        Ast_गाठ_DType* ध्रुव errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        यदि (ध्रुव Ast_गाठ_Array_DType* ध्रुव adtypep = VN_CAST(ddtypep, गाठ_Array_DType)) {
            fromRange = adtypep->declRange();
        } अन्यथा यदि (VN_IS(ddtypep, AssocArrayDType)) {
        } अन्यथा यदि (VN_IS(ddtypep, WildcardArrayDType)) {
        } अन्यथा यदि (VN_IS(ddtypep, DynArrayDType)) {
        } अन्यथा यदि (VN_IS(ddtypep, QueueDType)) {
        } अन्यथा यदि (ध्रुव Ast_गाठ_UOr_Struct_DType* ध्रुव adtypep
                   = VN_CAST(ddtypep, गाठ_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } अन्यथा यदि (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(ddtypep, BasicDType)) {
            यदि (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } अन्यथा यदि (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } अन्यथा {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } अन्यथा {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        प्रत्यागम FromData(errp, ddtypep, fromRange);
    }

    Ast_गाठ_अभिव्यक्ती* newSubNeg(Ast_गाठ_अभिव्यक्ती* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        यदि (rhs == 0) {
            प्रत्यागम lhsp;
        } अन्यथा यदि (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_संख्या अंक(lhsp, lhsp->width());
            अंक.opSub(VN_AS(lhsp, Const)->अंक(), V3_संख्या(lhsp, 32, rhs));
            अंक.isSigned(lhsp->isSigned());
            प्रत्यागम नूतन AstConst{lhsp->fileline(), अंक};
        } अन्यथा यदि (rhs > 0) {
            Ast_गाठ_अभिव्यक्ती* ध्रुव newp
                = नूतन AstSub{lhsp->fileline(), lhsp,
                             नूतन AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            प्रत्यागम newp;
        } अन्यथा {  // rhs < 0;
            Ast_गाठ_अभिव्यक्ती* ध्रुव newp
                = नूतन AstAdd{lhsp->fileline(), lhsp,
                             नूतन AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            प्रत्यागम newp;
        }
    }
    Ast_गाठ_अभिव्यक्ती* newSubNeg(int32_t lhs, Ast_गाठ_अभिव्यक्ती* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_गाठ_अभिव्यक्ती* ध्रुव newp = नूतन AstSub{
            rhsp->fileline(), नूतन AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        प्रत्यागम newp;
    }

    Ast_गाठ_अभिव्यक्ती* newSubLsbOf(Ast_गाठ_अभिव्यक्ती* underp, ध्रुव VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        यदि (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            प्रत्यागम underp;
        } अन्यथा {
            यदि (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                प्रत्यागम newSubNeg(fromRange.hi(), underp);
            } अन्यथा {
                // reg [3:1] needs a SUB(under,1)
                प्रत्यागम newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_गाठ_अभिव्यक्ती* newMulConst(File_ओळ* fl, uint32_t elwidth, Ast_गाठ_अभिव्यक्ती* indexp) {
        Ast_गाठ_अभिव्यक्ती* extendp;
        ध्रुव पूर्णाङ्क widthMin = std::max(V3_संख्या::log2b(elwidth) + 1, indexp->widthMin());
        यदि (indexp->width() > 32) {
            extendp = नूतन AstSel{fl, indexp, 0, 32, widthMin};
        } अन्यथा {
            extendp = नूतन AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_गाठ_अभिव्यक्ती* ध्रुव mulp
            = नूतन AstMul{fl, नूतन AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        प्रत्यागम mulp;
    }

    Ast_गाठ_DType* sliceDType(AstPackArrayDType* nodep, पूर्णाङ्क msb, पूर्णाङ्क lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        यदि (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            प्रत्यागम nodep;
        } अन्यथा {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ध्रुव VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_गाठ_DType* ध्रुव vardtypep
                = नूतन AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        नूतन AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            प्रत्यागम vardtypep;
        }
    }

    Ast_गाठ_अभिव्यक्ती* selQueueBackness(Ast_गाठ* nodep) {
        यदि (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            प्रत्यागम नूतन AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } अन्यथा यदि (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_गाठ_अभिव्यक्ती* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            प्रत्यागम subrhsp;
        } अन्यथा {
            प्रत्यागम nullptr;
        }
    }

    शून्य warnTri(Ast_गाठ* nodep) {
        यदि (VN_IS(nodep, Const) && VN_AS(nodep, Const)->अंक().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    शून्य visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_गाठ_अभिव्यक्ती* ध्रुव fromp = nodep->fromp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* ध्रुव rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ध्रुव FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_गाठ_DType* ध्रुव ddtypep = fromdata.m_dtypep;
        ध्रुव VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_गाठ_अभिव्यक्ती* subp = rhsp;
            यदि (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (ध्रुव AstPackArrayDType* ध्रुव adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_गाठ_अभिव्यक्ती* subp = rhsp;
            यदि (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } अन्यथा {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ध्रुव पूर्णाङ्क elwidth = adtypep->width() / fromRange.elements();
            AstSel* ध्रुव newp = नूतन AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            यदि (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(असत्य));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (ध्रुव AstAssocArrayDType* ध्रुव adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ध्रुव newp = नूतन AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (ध्रुव AstWildcardArrayDType* ध्रुव adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ध्रुव newp = नूतन AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (ध्रुव AstDynArrayDType* ध्रुव adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ध्रुव VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ध्रुव newp
                = नूतन AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (ध्रुव AstQueueDType* ध्रुव adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            यदि (Ast_गाठ_अभिव्यक्ती* ध्रुव backnessp = selQueueBackness(rhsp)) {
                ध्रुव VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = नूतन AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } अन्यथा {
                ध्रुव VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = नूतन AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_गाठ_अभिव्यक्ती* exprp = fromp;
            यावत् (सत्य) {
                यदि (AstMemberSel* ध्रुव memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } अन्यथा यदि (AstStructSel* ध्रुव structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } अन्यथा {
                    भङ्ग;
                }
            }
            ध्रुव Ast_गाठ_Var_Ref* ध्रुव varrefp = VN_CAST(exprp, गाठ_Var_Ref);
            Ast_गाठ_अभिव्यक्ती* newp;
            यदि (!varrefp || varrefp->access().isReadOnly()) {
                newp = नूतन AstGetcN{nodep->fileline(), fromp, rhsp};
            } अन्यथा {
                newp = नूतन AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ध्रुव newp
                = नूतन AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (VN_IS(ddtypep, गाठ_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ध्रुव newp
                = नूतन AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
        यदि (!rhsp->backp()) VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
    }
    शून्य visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_गाठ_अभिव्यक्ती* ध्रुव fromp = nodep->fromp()->unlinkFrBack();
        ध्रुव FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_गाठ_DType* ध्रुव ddtypep = fromdata.m_dtypep;
        ध्रुव VNumRange fromRange = fromdata.m_fromRange;
        यदि (VN_IS(ddtypep, QueueDType)) {
            Ast_गाठ_अभिव्यक्ती* ध्रुव qleftp = nodep->leftp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* ध्रुव qrightp = nodep->rightp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* ध्रुव qleftBacknessp = selQueueBackness(qleftp);
            Ast_गाठ_अभिव्यक्ती* ध्रुव qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ध्रुव VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* ध्रुव newp = नूतन AstCMethodHard{nodep->fileline(), fromp, method};
            यदि (qleftBacknessp) {
                VL_DO_DANGLING(ढकला_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } अन्यथा {
                newp->addPinsp(qleftp);
            }
            यदि (qrightBacknessp) {
                VL_DO_DANGLING(ढकला_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } अन्यथा {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(सत्य);
            newp->protect(असत्य);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_गाठ_अभिव्यक्ती* ध्रुव msbp = nodep->leftp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* ध्रुव lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        ध्रुव int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        यदि (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            यदि (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा यदि (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा {  // Slice
                AstSliceSel* ध्रुव newp
                    = नूतन AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        } अन्यथा यदि (AstPackArrayDType* ध्रुव adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            यदि (fromRange.ascending()) std::swap(msb, lsb);
            यदि (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            ध्रुव पूर्णाङ्क elwidth = adtypep->width() / fromRange.elements();
            AstSel* ध्रुव newp
                = नूतन AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            यदि (fromRange.ascending()) std::swap(msb, lsb);
            यदि (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ध्रुव newp = नूतन AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा यदि (VN_IS(ddtypep, गाठ_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            यदि (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ध्रुव newp = नूतन AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        यदि (!fromp->backp()) VL_DO_DANGLING(ढकला_Deletep(fromp), fromp);
        यदि (!msbp->backp()) VL_DO_DANGLING(ढकला_Deletep(msbp), msbp);
        यदि (!lsbp->backp()) VL_DO_DANGLING(ढकला_Deletep(lsbp), lsbp);
    }

    शून्य replaceSelPlusMinus(Ast_गाठ_Pre_Sel* nodep) {
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
        Ast_गाठ_अभिव्यक्ती* ध्रुव fromp = nodep->fromp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* ध्रुव widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        ध्रुव पूर्णाङ्क width = VN_AS(widthp, Const)->toSInt();
        यदि (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        यदि (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ध्रुव FromData fromdata = fromDataForArray(nodep, fromp);
        ध्रुव Ast_गाठ_DType* ध्रुव ddtypep = fromdata.m_dtypep;
        ध्रुव VNumRange fromRange = fromdata.m_fromRange;
        यदि (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            यदि (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा यदि (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा यदि (VN_IS(rhsp, Const)) {  // Slice
                ध्रुव int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ध्रुव int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                ध्रुव int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* ध्रुव newp = नूतन AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } अन्यथा यदि (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, गाठ_UOr_Struct_DType)
                       && VN_AS(ddtypep, गाठ_UOr_Struct_DType)->packedUnsup())) {
            पूर्णाङ्क elwidth = 1;
            पूर्णाङ्क newwidth = width;
            यदि (ध्रुव AstPackArrayDType* ध्रुव adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_गाठ_अभिव्यक्ती* newlsbp = nullptr;
            यदि (VN_IS(nodep, SelPlus)) {
                यदि (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } अन्यथा {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } अन्यथा यदि (VN_IS(nodep, SelMinus)) {
                यदि (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } अन्यथा {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } अन्यथा {
                nodep->v3fatalSrc("Bad Case");
            }
            यदि (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ध्रुव newp = नूतन AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        यदि (!fromp->backp()) VL_DO_DANGLING(ढकला_Deletep(fromp), fromp);
        यदि (!rhsp->backp()) VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
        यदि (!widthp->backp()) VL_DO_DANGLING(ढकला_Deletep(widthp), widthp);
    }
    शून्य visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    शून्य visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    शून्य visit(Ast_गाठ* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

सार्वजनिक:
    // CONSTRUCTORS
    WidthSelVisitor() = स्वभाव;
    ~WidthSelVisitor() override = स्वभाव;
    Ast_गाठ* mainAcceptEdit(Ast_गाठ* nodep) { प्रत्यागम iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_गाठ* V3Width::widthSelNoIterEdit(Ast_गाठ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    प्रत्यागम nodep;
}

#undef iterateChildren
