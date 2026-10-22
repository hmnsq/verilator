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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

ವರ್ಗ WidthSelVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    ಶೂನ್ಯ checkConstantOrReplace(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        ಯದಿ (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    ಸಂರಚನಾ FromData final {
        Ast_ಗಂಟು_DType* ಧ್ರುವ m_errp;  // Node that was found, for error reporting if not known type
        Ast_ಗಂಟು_DType* ಧ್ರುವ m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_ಗಂಟು_DType* errp, Ast_ಗಂಟು_DType* dtypep, ಧ್ರುವ VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = ಸ್ವಭಾವ;
    };
    ಸ್ಥಿರ FromData fromDataForArray(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        ಯಾವತ್ (basefromp) {
            ಯದಿ (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                ಅನುವರ್ತ;
            }
            ಭಙ್ಗ;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_ಗಂಟು_DType* ಧ್ರುವ ddtypep = basefromp->dtypep()->skipRefp();
        Ast_ಗಂಟು_DType* ಧ್ರುವ errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, ಗಂಟು_Array_DType)) {
            fromRange = adtypep->declRange();
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, AssocArrayDType)) {
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, WildcardArrayDType)) {
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, DynArrayDType)) {
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, QueueDType)) {
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ adtypep
                   = VN_CAST(ddtypep, ಗಂಟು_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, BasicDType)) {
            ಯದಿ (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } ಅನ್ಯಥಾ ಯದಿ (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } ಅನ್ಯಥಾ {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } ಅನ್ಯಥಾ {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        ಪ್ರತ್ಯಾಗಮ FromData(errp, ddtypep, fromRange);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSubNeg(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        ಯದಿ (rhs == 0) {
            ಪ್ರತ್ಯಾಗಮ lhsp;
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ(lhsp, lhsp->width());
            ಅಂಕೆ.opSub(VN_AS(lhsp, Const)->ಅಂಕೆ(), V3_ಸಂಖ್ಯೆ(lhsp, 32, rhs));
            ಅಂಕೆ.isSigned(lhsp->isSigned());
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{lhsp->fileline(), ಅಂಕೆ};
        } ಅನ್ಯಥಾ ಯದಿ (rhs > 0) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                = ನೂತನ AstSub{lhsp->fileline(), lhsp,
                             ನೂತನ AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ಪ್ರತ್ಯಾಗಮ newp;
        } ಅನ್ಯಥಾ {  // rhs < 0;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                = ನೂತನ AstAdd{lhsp->fileline(), lhsp,
                             ನೂತನ AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ಪ್ರತ್ಯಾಗಮ newp;
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSubNeg(int32_t lhs, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstSub{
            rhsp->fileline(), ನೂತನ AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSubLsbOf(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* underp, ಧ್ರುವ VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        ಯದಿ (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            ಪ್ರತ್ಯಾಗಮ underp;
        } ಅನ್ಯಥಾ {
            ಯದಿ (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                ಪ್ರತ್ಯಾಗಮ newSubNeg(fromRange.hi(), underp);
            } ಅನ್ಯಥಾ {
                // reg [3:1] needs a SUB(under,1)
                ಪ್ರತ್ಯಾಗಮ newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newMulConst(File_ಸಾಲು* fl, uint32_t elwidth, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* extendp;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = std::max(V3_ಸಂಖ್ಯೆ::log2b(elwidth) + 1, indexp->widthMin());
        ಯದಿ (indexp->width() > 32) {
            extendp = ನೂತನ AstSel{fl, indexp, 0, 32, widthMin};
        } ಅನ್ಯಥಾ {
            extendp = ನೂತನ AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ mulp
            = ನೂತನ AstMul{fl, ನೂತನ AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        ಪ್ರತ್ಯಾಗಮ mulp;
    }

    Ast_ಗಂಟು_DType* sliceDType(AstPackArrayDType* nodep, ಪೂರ್ಣಾಙ್ಕ msb, ಪೂರ್ಣಾಙ್ಕ lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        ಯದಿ (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            ಪ್ರತ್ಯಾಗಮ nodep;
        } ಅನ್ಯಥಾ {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ಧ್ರುವ VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_ಗಂಟು_DType* ಧ್ರುವ vardtypep
                = ನೂತನ AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        ನೂತನ AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            ಪ್ರತ್ಯಾಗಮ vardtypep;
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selQueueBackness(Ast_ಗಂಟು* nodep) {
        ಯದಿ (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            ಪ್ರತ್ಯಾಗಮ subrhsp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
    }

    ಶೂನ್ಯ warnTri(Ast_ಗಂಟು* nodep) {
        ಯದಿ (VN_IS(nodep, Const) && VN_AS(nodep, Const)->ಅಂಕೆ().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    ಶೂನ್ಯ visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ಧ್ರುವ FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_ಗಂಟು_DType* ಧ್ರುವ ddtypep = fromdata.m_dtypep;
        ಧ್ರುವ VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subp = rhsp;
            ಯದಿ (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ಧ್ರುವ newp = ನೂತನ AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subp = rhsp;
            ಯದಿ (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } ಅನ್ಯಥಾ {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elwidth = adtypep->width() / fromRange.elements();
            AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            ಯದಿ (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(ಅಸತ್ಯ));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ಧ್ರುವ newp = ನೂತನ AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstWildcardArrayDType* ಧ್ರುವ adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ಧ್ರುವ newp = ನೂತನ AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ಧ್ರುವ VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ಧ್ರುವ newp
                = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstQueueDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ backnessp = selQueueBackness(rhsp)) {
                ಧ್ರುವ VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = fromp;
            ಯಾವತ್ (ಸತ್ಯ) {
                ಯದಿ (AstMemberSel* ಧ್ರುವ memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } ಅನ್ಯಥಾ ಯದಿ (AstStructSel* ಧ್ರುವ structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } ಅನ್ಯಥಾ {
                    ಭಙ್ಗ;
                }
            }
            ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(exprp, ಗಂಟು_Var_Ref);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಯದಿ (!varrefp || varrefp->access().isReadOnly()) {
                newp = ನೂತನ AstGetcN{nodep->fileline(), fromp, rhsp};
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ಧ್ರುವ newp
                = ನೂತನ AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, ಗಂಟು_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ಧ್ರುವ newp
                = ನೂತನ AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        ಯದಿ (!rhsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
    }
    ಶೂನ್ಯ visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp()->unlinkFrBack();
        ಧ್ರುವ FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_ಗಂಟು_DType* ಧ್ರುವ ddtypep = fromdata.m_dtypep;
        ಧ್ರುವ VNumRange fromRange = fromdata.m_fromRange;
        ಯದಿ (VN_IS(ddtypep, QueueDType)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ qleftp = nodep->leftp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ qrightp = nodep->rightp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ qleftBacknessp = selQueueBackness(qleftp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ಧ್ರುವ VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* ಧ್ರುವ newp = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, method};
            ಯದಿ (qleftBacknessp) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } ಅನ್ಯಥಾ {
                newp->addPinsp(qleftp);
            }
            ಯದಿ (qrightBacknessp) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } ಅನ್ಯಥಾ {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(ಸತ್ಯ);
            newp->protect(ಅಸತ್ಯ);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ msbp = nodep->leftp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        ಧ್ರುವ int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        ಯದಿ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            ಯದಿ (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ಧ್ರುವ newp = ನೂತನ AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ {  // Slice
                AstSliceSel* ಧ್ರುವ newp
                    = ನೂತನ AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstPackArrayDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            ಯದಿ (fromRange.ascending()) std::swap(msb, lsb);
            ಯದಿ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elwidth = adtypep->width() / fromRange.elements();
            AstSel* ಧ್ರುವ newp
                = ನೂತನ AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            ಯದಿ (fromRange.ascending()) std::swap(msb, lsb);
            ಯದಿ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, ಗಂಟು_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            ಯದಿ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ಯದಿ (!fromp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        ಯದಿ (!msbp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(msbp), msbp);
        ಯದಿ (!lsbp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(lsbp), lsbp);
    }

    ಶೂನ್ಯ replaceSelPlusMinus(Ast_ಗಂಟು_Pre_Sel* nodep) {
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
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = VN_AS(widthp, Const)->toSInt();
        ಯದಿ (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        ಯದಿ (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ಧ್ರುವ FromData fromdata = fromDataForArray(nodep, fromp);
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ ddtypep = fromdata.m_dtypep;
        ಧ್ರುವ VNumRange fromRange = fromdata.m_fromRange;
        ಯದಿ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            ಯದಿ (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ಧ್ರುವ newp = ನೂತನ AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(rhsp, Const)) {  // Slice
                ಧ್ರುವ int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ಧ್ರುವ int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                ಧ್ರುವ int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* ಧ್ರುವ newp = ನೂತನ AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, ಗಂಟು_UOr_Struct_DType)
                       && VN_AS(ddtypep, ಗಂಟು_UOr_Struct_DType)->packedUnsup())) {
            ಪೂರ್ಣಾಙ್ಕ elwidth = 1;
            ಪೂರ್ಣಾಙ್ಕ newwidth = width;
            ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newlsbp = nullptr;
            ಯದಿ (VN_IS(nodep, SelPlus)) {
                ಯದಿ (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } ಅನ್ಯಥಾ {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, SelMinus)) {
                ಯದಿ (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } ಅನ್ಯಥಾ {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Bad Case");
            }
            ಯದಿ (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ಯದಿ (!fromp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        ಯದಿ (!rhsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
        ಯದಿ (!widthp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(widthp), widthp);
    }
    ಶೂನ್ಯ visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    ಶೂನ್ಯ visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    WidthSelVisitor() = ಸ್ವಭಾವ;
    ~WidthSelVisitor() override = ಸ್ವಭಾವ;
    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_ಗಂಟು* V3Width::widthSelNoIterEdit(Ast_ಗಂಟು* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    ಪ್ರತ್ಯಾಗಮ nodep;
}

#undef iterateChildren
