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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

শ্ৰেণী WidthSelVisitor final : সার্ৱজনিক VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    শূন্য checkConstantOrReplace(Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        যদি (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    সংরচনা FromData final {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ m_errp;  // Node that was found, for error reporting if not known type
        Ast_গ্ৰন্থি_DType* ধ্রুৱ m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_গ্ৰন্থি_DType* errp, Ast_গ্ৰন্থি_DType* dtypep, ধ্রুৱ VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = স্ৱভাৱ;
    };
    স্থির FromData fromDataForArray(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        যেতিয়ালৈ (basefromp) {
            যদি (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                অনুৱর্ত;
            }
            ভঙ্গ;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_গ্ৰন্থি_DType* ধ্রুৱ ddtypep = basefromp->dtypep()->skipRefp();
        Ast_গ্ৰন্থি_DType* ধ্রুৱ errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ adtypep = VN_CAST(ddtypep, গ্ৰন্থি_Array_DType)) {
            fromRange = adtypep->declRange();
        } অন্যথা যদি (VN_IS(ddtypep, AssocArrayDType)) {
        } অন্যথা যদি (VN_IS(ddtypep, WildcardArrayDType)) {
        } অন্যথা যদি (VN_IS(ddtypep, DynArrayDType)) {
        } অন্যথা যদি (VN_IS(ddtypep, QueueDType)) {
        } অন্যথা যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ adtypep
                   = VN_CAST(ddtypep, গ্ৰন্থি_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } অন্যথা যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, BasicDType)) {
            যদি (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } অন্যথা যদি (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } অন্যথা {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } অন্যথা {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        ঘূৰাই FromData(errp, ddtypep, fromRange);
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* newSubNeg(Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        যদি (rhs == 0) {
            ঘূৰাই lhsp;
        } অন্যথা যদি (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_সংখ্যা অঙ্ক(lhsp, lhsp->width());
            অঙ্ক.opSub(VN_AS(lhsp, Const)->অঙ্ক(), V3_সংখ্যা(lhsp, 32, rhs));
            অঙ্ক.isSigned(lhsp->isSigned());
            ঘূৰাই নূতন AstConst{lhsp->fileline(), অঙ্ক};
        } অন্যথা যদি (rhs > 0) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
                = নূতন AstSub{lhsp->fileline(), lhsp,
                             নূতন AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ঘূৰাই newp;
        } অন্যথা {  // rhs < 0;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
                = নূতন AstAdd{lhsp->fileline(), lhsp,
                             নূতন AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ঘূৰাই newp;
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* newSubNeg(int32_t lhs, Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstSub{
            rhsp->fileline(), নূতন AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        ঘূৰাই newp;
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* newSubLsbOf(Ast_গ্ৰন্থি_ৰাশিমালা* underp, ধ্রুৱ VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        যদি (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            ঘূৰাই underp;
        } অন্যথা {
            যদি (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                ঘূৰাই newSubNeg(fromRange.hi(), underp);
            } অন্যথা {
                // reg [3:1] needs a SUB(under,1)
                ঘূৰাই newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* newMulConst(File_সাৰি* fl, uint32_t elwidth, Ast_গ্ৰন্থি_ৰাশিমালা* indexp) {
        Ast_গ্ৰন্থি_ৰাশিমালা* extendp;
        ধ্রুৱ পূর্ণাঙ্ক widthMin = std::max(V3_সংখ্যা::log2b(elwidth) + 1, indexp->widthMin());
        যদি (indexp->width() > 32) {
            extendp = নূতন AstSel{fl, indexp, 0, 32, widthMin};
        } অন্যথা {
            extendp = নূতন AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ mulp
            = নূতন AstMul{fl, নূতন AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        ঘূৰাই mulp;
    }

    Ast_গ্ৰন্থি_DType* sliceDType(AstPackArrayDType* nodep, পূর্ণাঙ্ক msb, পূর্ণাঙ্ক lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        যদি (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            ঘূৰাই nodep;
        } অন্যথা {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ধ্রুৱ VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_গ্ৰন্থি_DType* ধ্রুৱ vardtypep
                = নূতন AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        নূতন AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            ঘূৰাই vardtypep;
        }
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* selQueueBackness(Ast_গ্ৰন্থি* nodep) {
        যদি (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            ঘূৰাই নূতন AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } অন্যথা যদি (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_গ্ৰন্থি_ৰাশিমালা* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            ঘূৰাই subrhsp;
        } অন্যথা {
            ঘূৰাই nullptr;
        }
    }

    শূন্য warnTri(Ast_গ্ৰন্থি* nodep) {
        যদি (VN_IS(nodep, Const) && VN_AS(nodep, Const)->অঙ্ক().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    শূন্য visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ধ্রুৱ FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ ddtypep = fromdata.m_dtypep;
        ধ্রুৱ VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_গ্ৰন্থি_ৰাশিমালা* subp = rhsp;
            যদি (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ধ্রুৱ newp = নূতন AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (ধ্রুৱ AstPackArrayDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_গ্ৰন্থি_ৰাশিমালা* subp = rhsp;
            যদি (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } অন্যথা {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ধ্রুৱ পূর্ণাঙ্ক elwidth = adtypep->width() / fromRange.elements();
            AstSel* ধ্রুৱ newp = নূতন AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            যদি (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(মিছা));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (ধ্রুৱ AstAssocArrayDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ধ্রুৱ newp = নূতন AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (ধ্রুৱ AstWildcardArrayDType* ধ্রুৱ adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ধ্রুৱ newp = নূতন AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (ধ্রুৱ AstDynArrayDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ধ্রুৱ VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ধ্রুৱ newp
                = নূতন AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (ধ্রুৱ AstQueueDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ backnessp = selQueueBackness(rhsp)) {
                ধ্রুৱ VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = নূতন AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } অন্যথা {
                ধ্রুৱ VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = নূতন AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_গ্ৰন্থি_ৰাশিমালা* exprp = fromp;
            যেতিয়ালৈ (সঁচা) {
                যদি (AstMemberSel* ধ্রুৱ memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } অন্যথা যদি (AstStructSel* ধ্রুৱ structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } অন্যথা {
                    ভঙ্গ;
                }
            }
            ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(exprp, গ্ৰন্থি_Var_Ref);
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            যদি (!varrefp || varrefp->access().isReadOnly()) {
                newp = নূতন AstGetcN{nodep->fileline(), fromp, rhsp};
            } অন্যথা {
                newp = নূতন AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ধ্রুৱ newp
                = নূতন AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (VN_IS(ddtypep, গ্ৰন্থি_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ধ্রুৱ newp
                = নূতন AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        যদি (!rhsp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
    }
    শূন্য visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp()->unlinkFrBack();
        ধ্রুৱ FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ ddtypep = fromdata.m_dtypep;
        ধ্রুৱ VNumRange fromRange = fromdata.m_fromRange;
        যদি (VN_IS(ddtypep, QueueDType)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ qleftp = nodep->leftp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ qrightp = nodep->rightp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ qleftBacknessp = selQueueBackness(qleftp);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ধ্রুৱ VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* ধ্রুৱ newp = নূতন AstCMethodHard{nodep->fileline(), fromp, method};
            যদি (qleftBacknessp) {
                VL_DO_DANGLING(ঠেলো_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } অন্যথা {
                newp->addPinsp(qleftp);
            }
            যদি (qrightBacknessp) {
                VL_DO_DANGLING(ঠেলো_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } অন্যথা {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(সঁচা);
            newp->protect(মিছা);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ msbp = nodep->leftp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        ধ্রুৱ int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        যদি (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            যদি (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা যদি (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ধ্রুৱ newp = নূতন AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা {  // Slice
                AstSliceSel* ধ্রুৱ newp
                    = নূতন AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        } অন্যথা যদি (AstPackArrayDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            যদি (fromRange.ascending()) std::swap(msb, lsb);
            যদি (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            ধ্রুৱ পূর্ণাঙ্ক elwidth = adtypep->width() / fromRange.elements();
            AstSel* ধ্রুৱ newp
                = নূতন AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            যদি (fromRange.ascending()) std::swap(msb, lsb);
            যদি (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ধ্রুৱ newp = নূতন AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (VN_IS(ddtypep, গ্ৰন্থি_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            যদি (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* ধ্রুৱ newp = নূতন AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        যদি (!fromp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(fromp), fromp);
        যদি (!msbp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(msbp), msbp);
        যদি (!lsbp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(lsbp), lsbp);
    }

    শূন্য replaceSelPlusMinus(Ast_গ্ৰন্থি_Pre_Sel* nodep) {
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
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        ধ্রুৱ পূর্ণাঙ্ক width = VN_AS(widthp, Const)->toSInt();
        যদি (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        যদি (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ধ্রুৱ FromData fromdata = fromDataForArray(nodep, fromp);
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ ddtypep = fromdata.m_dtypep;
        ধ্রুৱ VNumRange fromRange = fromdata.m_fromRange;
        যদি (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            যদি (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা যদি (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ধ্রুৱ newp = নূতন AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা যদি (VN_IS(rhsp, Const)) {  // Slice
                ধ্রুৱ int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ধ্রুৱ int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                ধ্রুৱ int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* ধ্রুৱ newp = নূতন AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } অন্যথা যদি (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, গ্ৰন্থি_UOr_Struct_DType)
                       && VN_AS(ddtypep, গ্ৰন্থি_UOr_Struct_DType)->packedUnsup())) {
            পূর্ণাঙ্ক elwidth = 1;
            পূর্ণাঙ্ক newwidth = width;
            যদি (ধ্রুৱ AstPackArrayDType* ধ্রুৱ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_গ্ৰন্থি_ৰাশিমালা* newlsbp = nullptr;
            যদি (VN_IS(nodep, SelPlus)) {
                যদি (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } অন্যথা {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } অন্যথা যদি (VN_IS(nodep, SelMinus)) {
                যদি (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } অন্যথা {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } অন্যথা {
                nodep->v3fatalSrc("Bad Case");
            }
            যদি (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ধ্রুৱ newp = নূতন AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        যদি (!fromp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(fromp), fromp);
        যদি (!rhsp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
        যদি (!widthp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(widthp), widthp);
    }
    শূন্য visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    শূন্য visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    WidthSelVisitor() = স্ৱভাৱ;
    ~WidthSelVisitor() override = স্ৱভাৱ;
    Ast_গ্ৰন্থি* mainAcceptEdit(Ast_গ্ৰন্থি* nodep) { ঘূৰাই iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_গ্ৰন্থি* V3Width::widthSelNoIterEdit(Ast_গ্ৰন্থি* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    ঘূৰাই nodep;
}

#undef iterateChildren
