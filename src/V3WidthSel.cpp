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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Const.h"
#समावेश "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

श्रेणी WidthSelVisitor final : खुला VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#घोषणा iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    व्योम checkConstantOrReplace(Ast_गाँठ* nodep, स्थिर string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        अगर (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    काष्ठा From_तथ्य final {
        Ast_गाँठ_DType* स्थिर m_errp;  // Node that was found, for error reporting if not known type
        Ast_गाँठ_DType* स्थिर m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        From_तथ्य(Ast_गाँठ_DType* errp, Ast_गाँठ_DType* dtypep, स्थिर VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~From_तथ्य() = शेष;
    };
    अटल From_तथ्य from_तथ्य_For_Array(Ast_गाँठ* nodep, Ast_गाँठ* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        जबतक (basefromp) {
            अगर (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                जारी;
            }
            अवरोध;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_गाँठ_DType* स्थिर ddtypep = basefromp->dtypep()->skipRefp();
        Ast_गाँठ_DType* स्थिर errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        अगर (स्थिर Ast_गाँठ_Array_DType* स्थिर adtypep = VN_CAST(ddtypep, गाँठ_Array_DType)) {
            fromRange = adtypep->declRange();
        } अन्यथा अगर (VN_IS(ddtypep, AssocArrayDType)) {
        } अन्यथा अगर (VN_IS(ddtypep, WildcardArrayDType)) {
        } अन्यथा अगर (VN_IS(ddtypep, DynArrayDType)) {
        } अन्यथा अगर (VN_IS(ddtypep, QueueDType)) {
        } अन्यथा अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर adtypep
                   = VN_CAST(ddtypep, गाँठ_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } अन्यथा अगर (स्थिर AstBasicDType* स्थिर adtypep = VN_CAST(ddtypep, BasicDType)) {
            अगर (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } अन्यथा अगर (adtypep->isRanged()) {
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
        वापस From_तथ्य(errp, ddtypep, fromRange);
    }

    Ast_गाँठ_व्यंजक* newSubNeg(Ast_गाँठ_व्यंजक* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        अगर (rhs == 0) {
            वापस lhsp;
        } अन्यथा अगर (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_संख्या अंक(lhsp, lhsp->width());
            अंक.opSub(VN_AS(lhsp, Const)->अंक(), V3_संख्या(lhsp, 32, rhs));
            अंक.isSigned(lhsp->isSigned());
            वापस नया AstConst{lhsp->fileline(), अंक};
        } अन्यथा अगर (rhs > 0) {
            Ast_गाँठ_व्यंजक* स्थिर newp
                = नया AstSub{lhsp->fileline(), lhsp,
                             नया AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            वापस newp;
        } अन्यथा {  // rhs < 0;
            Ast_गाँठ_व्यंजक* स्थिर newp
                = नया AstAdd{lhsp->fileline(), lhsp,
                             नया AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            वापस newp;
        }
    }
    Ast_गाँठ_व्यंजक* newSubNeg(int32_t lhs, Ast_गाँठ_व्यंजक* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_गाँठ_व्यंजक* स्थिर newp = नया AstSub{
            rhsp->fileline(), नया AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        वापस newp;
    }

    Ast_गाँठ_व्यंजक* newSubLsbOf(Ast_गाँठ_व्यंजक* underp, स्थिर VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        अगर (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            वापस underp;
        } अन्यथा {
            अगर (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                वापस newSubNeg(fromRange.hi(), underp);
            } अन्यथा {
                // reg [3:1] needs a SUB(under,1)
                वापस newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_गाँठ_व्यंजक* newMulConst(File_पंक्ति* fl, uint32_t elwidth, Ast_गाँठ_व्यंजक* indexp) {
        Ast_गाँठ_व्यंजक* extendp;
        स्थिर पूर्णांक widthMin = std::max(V3_संख्या::log2b(elwidth) + 1, indexp->widthMin());
        अगर (indexp->width() > 32) {
            extendp = नया AstSel{fl, indexp, 0, 32, widthMin};
        } अन्यथा {
            extendp = नया AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_गाँठ_व्यंजक* स्थिर mulp
            = नया AstMul{fl, नया AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        वापस mulp;
    }

    Ast_गाँठ_DType* sliceDType(AstPackArrayDType* nodep, पूर्णांक msb, पूर्णांक lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        अगर (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            वापस nodep;
        } अन्यथा {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            स्थिर VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_गाँठ_DType* स्थिर vardtypep
                = नया AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        नया AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            वापस vardtypep;
        }
    }

    Ast_गाँठ_व्यंजक* selQueueBackness(Ast_गाँठ* nodep) {
        अगर (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            वापस नया AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } अन्यथा अगर (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_गाँठ_व्यंजक* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            वापस subrhsp;
        } अन्यथा {
            वापस nullptr;
        }
    }

    व्योम warnTri(Ast_गाँठ* nodep) {
        अगर (VN_IS(nodep, Const) && VN_AS(nodep, Const)->अंक().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    व्योम visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_गाँठ_व्यंजक* स्थिर fromp = nodep->fromp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        स्थिर From_तथ्य fromdata = from_तथ्य_For_Array(nodep, fromp);
        Ast_गाँठ_DType* स्थिर ddtypep = fromdata.m_dtypep;
        स्थिर VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_गाँठ_व्यंजक* subp = rhsp;
            अगर (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* स्थिर newp = नया AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (स्थिर AstPackArrayDType* स्थिर adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_गाँठ_व्यंजक* subp = rhsp;
            अगर (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } अन्यथा {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            स्थिर पूर्णांक elwidth = adtypep->width() / fromRange.elements();
            AstSel* स्थिर newp = नया AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            अगर (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(false));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (स्थिर AstAssocArrayDType* स्थिर adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* स्थिर newp = नया AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (स्थिर AstWildcardArrayDType* स्थिर adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* स्थिर newp = नया AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (स्थिर AstDynArrayDType* स्थिर adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            स्थिर VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* स्थिर newp
                = नया AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (स्थिर AstQueueDType* स्थिर adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            अगर (Ast_गाँठ_व्यंजक* स्थिर backnessp = selQueueBackness(rhsp)) {
                स्थिर VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = नया AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } अन्यथा {
                स्थिर VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = नया AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_गाँठ_व्यंजक* exprp = fromp;
            जबतक (true) {
                अगर (AstMemberSel* स्थिर memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } अन्यथा अगर (AstStructSel* स्थिर structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } अन्यथा {
                    अवरोध;
                }
            }
            स्थिर Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(exprp, गाँठ_Var_Ref);
            Ast_गाँठ_व्यंजक* newp;
            अगर (!varrefp || varrefp->access().isReadOnly()) {
                newp = नया AstGetcN{nodep->fileline(), fromp, rhsp};
            } अन्यथा {
                newp = नया AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* स्थिर newp
                = नया AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (VN_IS(ddtypep, गाँठ_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* स्थिर newp
                = नया AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
        अगर (!rhsp->backp()) VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
    }
    व्योम visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_गाँठ_व्यंजक* स्थिर fromp = nodep->fromp()->unlinkFrBack();
        स्थिर From_तथ्य fromdata = from_तथ्य_For_Array(nodep, fromp);
        Ast_गाँठ_DType* स्थिर ddtypep = fromdata.m_dtypep;
        स्थिर VNumRange fromRange = fromdata.m_fromRange;
        अगर (VN_IS(ddtypep, QueueDType)) {
            Ast_गाँठ_व्यंजक* स्थिर qleftp = nodep->leftp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर qrightp = nodep->rightp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर qleftBacknessp = selQueueBackness(qleftp);
            Ast_गाँठ_व्यंजक* स्थिर qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            स्थिर VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* स्थिर newp = नया AstCMethodHard{nodep->fileline(), fromp, method};
            अगर (qleftBacknessp) {
                VL_DO_DANGLING(धकेलो_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } अन्यथा {
                newp->addPinsp(qleftp);
            }
            अगर (qrightBacknessp) {
                VL_DO_DANGLING(धकेलो_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } अन्यथा {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(true);
            newp->protect(false);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_गाँठ_व्यंजक* स्थिर msbp = nodep->leftp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        स्थिर int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        अगर (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            अगर (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा अगर (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* स्थिर newp = नया AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा {  // Slice
                AstSliceSel* स्थिर newp
                    = नया AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
        } अन्यथा अगर (AstPackArrayDType* स्थिर adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            अगर (fromRange.ascending()) std::swap(msb, lsb);
            अगर (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            स्थिर पूर्णांक elwidth = adtypep->width() / fromRange.elements();
            AstSel* स्थिर newp
                = नया AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            अगर (fromRange.ascending()) std::swap(msb, lsb);
            अगर (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* स्थिर newp = नया AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा अगर (VN_IS(ddtypep, गाँठ_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            अगर (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* स्थिर newp = नया AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        अगर (!fromp->backp()) VL_DO_DANGLING(धकेलो_Deletep(fromp), fromp);
        अगर (!msbp->backp()) VL_DO_DANGLING(धकेलो_Deletep(msbp), msbp);
        अगर (!lsbp->backp()) VL_DO_DANGLING(धकेलो_Deletep(lsbp), lsbp);
    }

    व्योम replaceSelPlusMinus(Ast_गाँठ_Pre_Sel* nodep) {
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
        Ast_गाँठ_व्यंजक* स्थिर fromp = nodep->fromp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        स्थिर पूर्णांक width = VN_AS(widthp, Const)->toSInt();
        अगर (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        अगर (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        स्थिर From_तथ्य fromdata = from_तथ्य_For_Array(nodep, fromp);
        स्थिर Ast_गाँठ_DType* स्थिर ddtypep = fromdata.m_dtypep;
        स्थिर VNumRange fromRange = fromdata.m_fromRange;
        अगर (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            अगर (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा अगर (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* स्थिर newp = नया AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा अगर (VN_IS(rhsp, Const)) {  // Slice
                स्थिर int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                स्थिर int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                स्थिर int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* स्थिर newp = नया AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } अन्यथा अगर (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, गाँठ_UOr_Struct_DType)
                       && VN_AS(ddtypep, गाँठ_UOr_Struct_DType)->packedUnsup())) {
            पूर्णांक elwidth = 1;
            पूर्णांक newwidth = width;
            अगर (स्थिर AstPackArrayDType* स्थिर adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_गाँठ_व्यंजक* newlsbp = nullptr;
            अगर (VN_IS(nodep, SelPlus)) {
                अगर (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } अन्यथा {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } अन्यथा अगर (VN_IS(nodep, SelMinus)) {
                अगर (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } अन्यथा {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } अन्यथा {
                nodep->v3fatalSrc("Bad Case");
            }
            अगर (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* स्थिर newp = नया AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        अगर (!fromp->backp()) VL_DO_DANGLING(धकेलो_Deletep(fromp), fromp);
        अगर (!rhsp->backp()) VL_DO_DANGLING(धकेलो_Deletep(rhsp), rhsp);
        अगर (!widthp->backp()) VL_DO_DANGLING(धकेलो_Deletep(widthp), widthp);
    }
    व्योम visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    व्योम visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    व्योम visit(Ast_गाँठ* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

खुला:
    // CONSTRUCTORS
    WidthSelVisitor() = शेष;
    ~WidthSelVisitor() override = शेष;
    Ast_गाँठ* mainAcceptEdit(Ast_गाँठ* nodep) { वापस iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_गाँठ* V3Width::widthSelNoIterEdit(Ast_गाँठ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    वापस nodep;
}

#अघोषित iterateChildren
