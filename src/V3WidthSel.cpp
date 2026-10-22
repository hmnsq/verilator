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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

வகுப்பு WidthSelVisitor final : பொது VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    வெறுமை checkConstantOrReplace(Ast_முனை* nodep, மாறிலி string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        எனில் (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    கட்டமைப்பு FromData final {
        Ast_முனை_DType* மாறிலி m_errp;  // Node that was found, for error reporting if not known type
        Ast_முனை_DType* மாறிலி m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_முனை_DType* errp, Ast_முனை_DType* dtypep, மாறிலி VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = இயல்பு;
    };
    நிலையான FromData fromDataForArray(Ast_முனை* nodep, Ast_முனை* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        போது (basefromp) {
            எனில் (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                தொடர்;
            }
            நிறுத்து;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_முனை_DType* மாறிலி ddtypep = basefromp->dtypep()->skipRefp();
        Ast_முனை_DType* மாறிலி errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        எனில் (மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(ddtypep, முனை_Array_DType)) {
            fromRange = adtypep->declRange();
        } இல்லையெனில் எனில் (VN_IS(ddtypep, AssocArrayDType)) {
        } இல்லையெனில் எனில் (VN_IS(ddtypep, WildcardArrayDType)) {
        } இல்லையெனில் எனில் (VN_IS(ddtypep, DynArrayDType)) {
        } இல்லையெனில் எனில் (VN_IS(ddtypep, QueueDType)) {
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி adtypep
                   = VN_CAST(ddtypep, முனை_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } இல்லையெனில் எனில் (மாறிலி AstBasicDType* மாறிலி adtypep = VN_CAST(ddtypep, BasicDType)) {
            எனில் (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } இல்லையெனில் எனில் (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } இல்லையெனில் {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } இல்லையெனில் {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        திரும்பு FromData(errp, ddtypep, fromRange);
    }

    Ast_முனை_கோவை* newSubNeg(Ast_முனை_கோவை* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        எனில் (rhs == 0) {
            திரும்பு lhsp;
        } இல்லையெனில் எனில் (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_எண் இலக்கம்(lhsp, lhsp->width());
            இலக்கம்.opSub(VN_AS(lhsp, Const)->இலக்கம்(), V3_எண்(lhsp, 32, rhs));
            இலக்கம்.isSigned(lhsp->isSigned());
            திரும்பு புதிய AstConst{lhsp->fileline(), இலக்கம்};
        } இல்லையெனில் எனில் (rhs > 0) {
            Ast_முனை_கோவை* மாறிலி newp
                = புதிய AstSub{lhsp->fileline(), lhsp,
                             புதிய AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            திரும்பு newp;
        } இல்லையெனில் {  // rhs < 0;
            Ast_முனை_கோவை* மாறிலி newp
                = புதிய AstAdd{lhsp->fileline(), lhsp,
                             புதிய AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            திரும்பு newp;
        }
    }
    Ast_முனை_கோவை* newSubNeg(int32_t lhs, Ast_முனை_கோவை* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_முனை_கோவை* மாறிலி newp = புதிய AstSub{
            rhsp->fileline(), புதிய AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        திரும்பு newp;
    }

    Ast_முனை_கோவை* newSubLsbOf(Ast_முனை_கோவை* underp, மாறிலி VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        எனில் (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            திரும்பு underp;
        } இல்லையெனில் {
            எனில் (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                திரும்பு newSubNeg(fromRange.hi(), underp);
            } இல்லையெனில் {
                // reg [3:1] needs a SUB(under,1)
                திரும்பு newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_முனை_கோவை* newMulConst(File_வரி* fl, uint32_t elwidth, Ast_முனை_கோவை* indexp) {
        Ast_முனை_கோவை* extendp;
        மாறிலி முழு widthMin = std::max(V3_எண்::log2b(elwidth) + 1, indexp->widthMin());
        எனில் (indexp->width() > 32) {
            extendp = புதிய AstSel{fl, indexp, 0, 32, widthMin};
        } இல்லையெனில் {
            extendp = புதிய AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_முனை_கோவை* மாறிலி mulp
            = புதிய AstMul{fl, புதிய AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        திரும்பு mulp;
    }

    Ast_முனை_DType* sliceDType(AstPackArrayDType* nodep, முழு msb, முழு lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        எனில் (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            திரும்பு nodep;
        } இல்லையெனில் {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            மாறிலி VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_முனை_DType* மாறிலி vardtypep
                = புதிய AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        புதிய AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            திரும்பு vardtypep;
        }
    }

    Ast_முனை_கோவை* selQueueBackness(Ast_முனை* nodep) {
        எனில் (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            திரும்பு புதிய AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } இல்லையெனில் எனில் (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_முனை_கோவை* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            திரும்பு subrhsp;
        } இல்லையெனில் {
            திரும்பு nullptr;
        }
    }

    வெறுமை warnTri(Ast_முனை* nodep) {
        எனில் (VN_IS(nodep, Const) && VN_AS(nodep, Const)->இலக்கம்().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    வெறுமை visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_முனை_கோவை* மாறிலி fromp = nodep->fromp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        மாறிலி FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_முனை_DType* மாறிலி ddtypep = fromdata.m_dtypep;
        மாறிலி VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_முனை_கோவை* subp = rhsp;
            எனில் (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* மாறிலி newp = புதிய AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (மாறிலி AstPackArrayDType* மாறிலி adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_முனை_கோவை* subp = rhsp;
            எனில் (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } இல்லையெனில் {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            மாறிலி முழு elwidth = adtypep->width() / fromRange.elements();
            AstSel* மாறிலி newp = புதிய AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            எனில் (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(பொய்));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (மாறிலி AstAssocArrayDType* மாறிலி adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* மாறிலி newp = புதிய AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (மாறிலி AstWildcardArrayDType* மாறிலி adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* மாறிலி newp = புதிய AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (மாறிலி AstDynArrayDType* மாறிலி adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            மாறிலி VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* மாறிலி newp
                = புதிய AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (மாறிலி AstQueueDType* மாறிலி adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            எனில் (Ast_முனை_கோவை* மாறிலி backnessp = selQueueBackness(rhsp)) {
                மாறிலி VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = புதிய AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } இல்லையெனில் {
                மாறிலி VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = புதிய AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_முனை_கோவை* exprp = fromp;
            போது (உண்மை) {
                எனில் (AstMemberSel* மாறிலி memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } இல்லையெனில் எனில் (AstStructSel* மாறிலி structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } இல்லையெனில் {
                    நிறுத்து;
                }
            }
            மாறிலி Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(exprp, முனை_Var_Ref);
            Ast_முனை_கோவை* newp;
            எனில் (!varrefp || varrefp->access().isReadOnly()) {
                newp = புதிய AstGetcN{nodep->fileline(), fromp, rhsp};
            } இல்லையெனில் {
                newp = புதிய AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* மாறிலி newp
                = புதிய AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (VN_IS(ddtypep, முனை_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* மாறிலி newp
                = புதிய AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
        எனில் (!rhsp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
    }
    வெறுமை visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_முனை_கோவை* மாறிலி fromp = nodep->fromp()->unlinkFrBack();
        மாறிலி FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_முனை_DType* மாறிலி ddtypep = fromdata.m_dtypep;
        மாறிலி VNumRange fromRange = fromdata.m_fromRange;
        எனில் (VN_IS(ddtypep, QueueDType)) {
            Ast_முனை_கோவை* மாறிலி qleftp = nodep->leftp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி qrightp = nodep->rightp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி qleftBacknessp = selQueueBackness(qleftp);
            Ast_முனை_கோவை* மாறிலி qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            மாறிலி VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* மாறிலி newp = புதிய AstCMethodHard{nodep->fileline(), fromp, method};
            எனில் (qleftBacknessp) {
                VL_DO_DANGLING(தள்ளு_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } இல்லையெனில் {
                newp->addPinsp(qleftp);
            }
            எனில் (qrightBacknessp) {
                VL_DO_DANGLING(தள்ளு_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } இல்லையெனில் {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(உண்மை);
            newp->protect(பொய்);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_முனை_கோவை* மாறிலி msbp = nodep->leftp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        மாறிலி int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        எனில் (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            எனில் (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* மாறிலி newp = புதிய AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் {  // Slice
                AstSliceSel* மாறிலி newp
                    = புதிய AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        } இல்லையெனில் எனில் (AstPackArrayDType* மாறிலி adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            எனில் (fromRange.ascending()) std::swap(msb, lsb);
            எனில் (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            மாறிலி முழு elwidth = adtypep->width() / fromRange.elements();
            AstSel* மாறிலி newp
                = புதிய AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            எனில் (fromRange.ascending()) std::swap(msb, lsb);
            எனில் (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* மாறிலி newp = புதிய AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் எனில் (VN_IS(ddtypep, முனை_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            எனில் (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* மாறிலி newp = புதிய AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        எனில் (!fromp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(fromp), fromp);
        எனில் (!msbp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(msbp), msbp);
        எனில் (!lsbp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(lsbp), lsbp);
    }

    வெறுமை replaceSelPlusMinus(Ast_முனை_Pre_Sel* nodep) {
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
        Ast_முனை_கோவை* மாறிலி fromp = nodep->fromp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        மாறிலி முழு width = VN_AS(widthp, Const)->toSInt();
        எனில் (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        எனில் (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        மாறிலி FromData fromdata = fromDataForArray(nodep, fromp);
        மாறிலி Ast_முனை_DType* மாறிலி ddtypep = fromdata.m_dtypep;
        மாறிலி VNumRange fromRange = fromdata.m_fromRange;
        எனில் (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            எனில் (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* மாறிலி newp = புதிய AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் எனில் (VN_IS(rhsp, Const)) {  // Slice
                மாறிலி int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                மாறிலி int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                மாறிலி int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* மாறிலி newp = புதிய AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } இல்லையெனில் எனில் (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, முனை_UOr_Struct_DType)
                       && VN_AS(ddtypep, முனை_UOr_Struct_DType)->packedUnsup())) {
            முழு elwidth = 1;
            முழு newwidth = width;
            எனில் (மாறிலி AstPackArrayDType* மாறிலி adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_முனை_கோவை* newlsbp = nullptr;
            எனில் (VN_IS(nodep, SelPlus)) {
                எனில் (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } இல்லையெனில் {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } இல்லையெனில் எனில் (VN_IS(nodep, SelMinus)) {
                எனில் (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } இல்லையெனில் {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } இல்லையெனில் {
                nodep->v3fatalSrc("Bad Case");
            }
            எனில் (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* மாறிலி newp = புதிய AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        எனில் (!fromp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(fromp), fromp);
        எனில் (!rhsp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(rhsp), rhsp);
        எனில் (!widthp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(widthp), widthp);
    }
    வெறுமை visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    வெறுமை visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    வெறுமை visit(Ast_முனை* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

பொது:
    // CONSTRUCTORS
    WidthSelVisitor() = இயல்பு;
    ~WidthSelVisitor() override = இயல்பு;
    Ast_முனை* mainAcceptEdit(Ast_முனை* nodep) { திரும்பு iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_முனை* V3Width::widthSelNoIterEdit(Ast_முனை* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    திரும்பு nodep;
}

#undef iterateChildren
