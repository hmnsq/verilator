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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

ଶ୍ରେଣୀ WidthSelVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    ଶୂନ୍ୟ checkConstantOrReplace(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        ଯଦି (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    ସଂରଚନା FromData final {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ m_errp;  // Node that was found, for error reporting if not known type
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_ଗଣ୍ଠି_DType* errp, Ast_ଗଣ୍ଠି_DType* dtypep, ଅଚଳ VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    };
    ସ୍ଥିର FromData fromDataForArray(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        ଯେତେବେଳେ (basefromp) {
            ଯଦି (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                ଜାରି;
            }
            ଭାଙ୍ଗ;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ ddtypep = basefromp->dtypep()->skipRefp();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ adtypep = VN_CAST(ddtypep, ଗଣ୍ଠି_Array_DType)) {
            fromRange = adtypep->declRange();
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, AssocArrayDType)) {
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, WildcardArrayDType)) {
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, DynArrayDType)) {
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, QueueDType)) {
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ adtypep
                   = VN_CAST(ddtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ adtypep = VN_CAST(ddtypep, BasicDType)) {
            ଯଦି (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } ଅନ୍ୟଥା ଯଦି (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } ଅନ୍ୟଥା {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } ଅନ୍ୟଥା {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        ଫେରନ୍ତୁ FromData(errp, ddtypep, fromRange);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSubNeg(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        ଯଦି (rhs == 0) {
            ଫେରନ୍ତୁ lhsp;
        } ଅନ୍ୟଥା ଯଦି (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_ସଂଖ୍ୟା ଅଙ୍କ(lhsp, lhsp->width());
            ଅଙ୍କ.opSub(VN_AS(lhsp, Const)->ଅଙ୍କ(), V3_ସଂଖ୍ୟା(lhsp, 32, rhs));
            ଅଙ୍କ.isSigned(lhsp->isSigned());
            ଫେରନ୍ତୁ ନୂତନ AstConst{lhsp->fileline(), ଅଙ୍କ};
        } ଅନ୍ୟଥା ଯଦି (rhs > 0) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                = ନୂତନ AstSub{lhsp->fileline(), lhsp,
                             ନୂତନ AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ଫେରନ୍ତୁ newp;
        } ଅନ୍ୟଥା {  // rhs < 0;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
                = ନୂତନ AstAdd{lhsp->fileline(), lhsp,
                             ନୂତନ AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ଫେରନ୍ତୁ newp;
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSubNeg(int32_t lhs, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstSub{
            rhsp->fileline(), ନୂତନ AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        ଫେରନ୍ତୁ newp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSubLsbOf(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* underp, ଅଚଳ VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        ଯଦି (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            ଫେରନ୍ତୁ underp;
        } ଅନ୍ୟଥା {
            ଯଦି (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                ଫେରନ୍ତୁ newSubNeg(fromRange.hi(), underp);
            } ଅନ୍ୟଥା {
                // reg [3:1] needs a SUB(under,1)
                ଫେରନ୍ତୁ newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newMulConst(File_ଧାଡ଼ି* fl, uint32_t elwidth, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* extendp;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = std::max(V3_ସଂଖ୍ୟା::log2b(elwidth) + 1, indexp->widthMin());
        ଯଦି (indexp->width() > 32) {
            extendp = ନୂତନ AstSel{fl, indexp, 0, 32, widthMin};
        } ଅନ୍ୟଥା {
            extendp = ନୂତନ AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ mulp
            = ନୂତନ AstMul{fl, ନୂତନ AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        ଫେରନ୍ତୁ mulp;
    }

    Ast_ଗଣ୍ଠି_DType* sliceDType(AstPackArrayDType* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        ଯଦି (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            ଫେରନ୍ତୁ nodep;
        } ଅନ୍ୟଥା {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ଅଚଳ VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ vardtypep
                = ନୂତନ AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        ନୂତନ AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            ଫେରନ୍ତୁ vardtypep;
        }
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* selQueueBackness(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            ଫେରନ୍ତୁ ନୂତନ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            ଫେରନ୍ତୁ subrhsp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nullptr;
        }
    }

    ଶୂନ୍ୟ warnTri(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (VN_IS(nodep, Const) && VN_AS(nodep, Const)->ଅଙ୍କ().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    ଶୂନ୍ୟ visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ଅଚଳ FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ ddtypep = fromdata.m_dtypep;
        ଅଚଳ VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subp = rhsp;
            ଯଦି (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ଅଚଳ newp = ନୂତନ AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* subp = rhsp;
            ଯଦି (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } ଅନ୍ୟଥା {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elwidth = adtypep->width() / fromRange.elements();
            AstSel* ଅଚଳ newp = ନୂତନ AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            ଯଦି (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(ମିଥ୍ୟା));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstAssocArrayDType* ଅଚଳ adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ଅଚଳ newp = ନୂତନ AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstWildcardArrayDType* ଅଚଳ adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ଅଚଳ newp = ନୂତନ AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstDynArrayDType* ଅଚଳ adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ଅଚଳ VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ଅଚଳ newp
                = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstQueueDType* ଅଚଳ adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ backnessp = selQueueBackness(rhsp)) {
                ଅଚଳ VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } ଅନ୍ୟଥା {
                ଅଚଳ VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = fromp;
            ଯେତେବେଳେ (ସତ୍ୟ) {
                ଯଦି (AstMemberSel* ଅଚଳ memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } ଅନ୍ୟଥା ଯଦି (AstStructSel* ଅଚଳ structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } ଅନ୍ୟଥା {
                    ଭାଙ୍ଗ;
                }
            }
            ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(exprp, ଗଣ୍ଠି_Var_Ref);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            ଯଦି (!varrefp || varrefp->access().isReadOnly()) {
                newp = ନୂତନ AstGetcN{nodep->fileline(), fromp, rhsp};
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ଅଚଳ newp
                = ନୂତନ AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ଅଚଳ newp
                = ନୂତନ AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
        ଯଦି (!rhsp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
    }
    ଶୂନ୍ୟ visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp()->unlinkFrBack();
        ଅଚଳ FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ ddtypep = fromdata.m_dtypep;
        ଅଚଳ VNumRange fromRange = fromdata.m_fromRange;
        ଯଦି (VN_IS(ddtypep, QueueDType)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ qleftp = nodep->leftp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ qrightp = nodep->rightp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ qleftBacknessp = selQueueBackness(qleftp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ଅଚଳ VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* ଅଚଳ newp = ନୂତନ AstCMethodHard{nodep->fileline(), fromp, method};
            ଯଦି (qleftBacknessp) {
                VL_DO_DANGLING(ଠେଲ_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } ଅନ୍ୟଥା {
                newp->addPinsp(qleftp);
            }
            ଯଦି (qrightBacknessp) {
                VL_DO_DANGLING(ଠେଲ_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } ଅନ୍ୟଥା {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(ସତ୍ୟ);
            newp->protect(ମିଥ୍ୟା);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ msbp = nodep->leftp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        ଅଚଳ int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        ଯଦି (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            ଯଦି (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ଅଚଳ newp = ନୂତନ AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା {  // Slice
                AstSliceSel* ଅଚଳ newp
                    = ନୂତନ AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        } ଅନ୍ୟଥା ଯଦି (AstPackArrayDType* ଅଚଳ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            ଯଦି (fromRange.ascending()) std::swap(msb, lsb);
            ଯଦି (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elwidth = adtypep->width() / fromRange.elements();
            AstSel* ଅଚଳ newp
                = ନୂତନ AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            ଯଦି (fromRange.ascending()) std::swap(msb, lsb);
            ଯଦି (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ଅଚଳ newp = ନୂତନ AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            ଯଦି (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ଅଚଳ newp = ନୂତନ AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ଯଦି (!fromp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(fromp), fromp);
        ଯଦି (!msbp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(msbp), msbp);
        ଯଦି (!lsbp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(lsbp), lsbp);
    }

    ଶୂନ୍ୟ replaceSelPlusMinus(Ast_ଗଣ୍ଠି_Pre_Sel* nodep) {
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
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = VN_AS(widthp, Const)->toSInt();
        ଯଦି (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        ଯଦି (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ଅଚଳ FromData fromdata = fromDataForArray(nodep, fromp);
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ ddtypep = fromdata.m_dtypep;
        ଅଚଳ VNumRange fromRange = fromdata.m_fromRange;
        ଯଦି (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            ଯଦି (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ଅଚଳ newp = ନୂତନ AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(rhsp, Const)) {  // Slice
                ଅଚଳ int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ଅଚଳ int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                ଅଚଳ int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* ଅଚଳ newp = ନୂତନ AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, ଗଣ୍ଠି_UOr_Struct_DType)
                       && VN_AS(ddtypep, ଗଣ୍ଠି_UOr_Struct_DType)->packedUnsup())) {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା elwidth = 1;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା newwidth = width;
            ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newlsbp = nullptr;
            ଯଦି (VN_IS(nodep, SelPlus)) {
                ଯଦି (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } ଅନ୍ୟଥା {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, SelMinus)) {
                ଯଦି (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } ଅନ୍ୟଥା {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Bad Case");
            }
            ଯଦି (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ଅଚଳ newp = ନୂତନ AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ଯଦି (!fromp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(fromp), fromp);
        ଯଦି (!rhsp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(rhsp), rhsp);
        ଯଦି (!widthp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(widthp), widthp);
    }
    ଶୂନ୍ୟ visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    ଶୂନ୍ୟ visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    WidthSelVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~WidthSelVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Ast_ଗଣ୍ଠି* mainAcceptEdit(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_ଗଣ୍ଠି* V3Width::widthSelNoIterEdit(Ast_ଗଣ୍ଠି* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    ଫେରନ୍ତୁ nodep;
}

#undef iterateChildren
