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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

తరగతి వెడల్పు_Sel_Visitor final : బహిరంగ VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#నిర్వచించు iterateChildren DO_NOT_iterate_Childern_లోపల_V3_వెడల్పు_Sel

    // METHODS

    శూన్య checkConstantOrReplace(Ast_కణుపు* nodep, స్థిరరాశి string& సందేశం) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        ఒకవేళ (!VN_IS(nodep, Const)) {
            nodep->v3error(సందేశం);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    నిర్మాణం From_దత్తాంశం final {
        Ast_కణుపు_DType* స్థిరరాశి m_errp;  // Node that was found, for error reporting if not known type
        Ast_కణుపు_DType* స్థిరరాశి m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        From_దత్తాంశం(Ast_కణుపు_DType* errp, Ast_కణుపు_DType* dtypep, స్థిరరాశి VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~From_దత్తాంశం() = అప్రమేయ;
    };
    స్థిర From_దత్తాంశం from_దత్తాంశం_For_Array(Ast_కణుపు* nodep, Ast_కణుపు* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        అయితే (basefromp) {
            ఒకవేళ (VN_IS(basefromp, Attr_యొక్క)) {
                basefromp = VN_AS(basefromp, Attr_యొక్క)->fromp();
                కొనసాగు;
            }
            ఆపు;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_కణుపు_DType* స్థిరరాశి ddtypep = basefromp->dtypep()->skipRefp();
        Ast_కణుపు_DType* స్థిరరాశి errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(ddtypep, కణుపు_Array_DType)) {
            fromRange = adtypep->declRange();
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, AssocArrayDType)) {
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, WildcardArrayDType)) {
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, DynArrayDType)) {
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, వరుస_DType)) {
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి adtypep
                   = VN_CAST(ddtypep, కణుపు_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, BasicDType)) {
            ఒకవేళ (adtypep->isString() && VN_IS(nodep, Sel_బిట్టు)) {
            } లేకపోతే ఒకవేళ (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } లేకపోతే {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } లేకపోతే {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        తిరిగి From_దత్తాంశం(errp, ddtypep, fromRange);
    }

    Ast_కణుపు_సమాసం* newSubNeg(Ast_కణుపు_సమాసం* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        ఒకవేళ (rhs == 0) {
            తిరిగి lhsp;
        } లేకపోతే ఒకవేళ (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_సంఖ్య అంకె(lhsp, lhsp->వెడల్పు());
            అంకె.opSub(VN_AS(lhsp, Const)->అంకె(), V3_సంఖ్య(lhsp, 32, rhs));
            అంకె.isSigned(lhsp->isSigned());
            తిరిగి కొత్త AstConst{lhsp->fileline(), అంకె};
        } లేకపోతే ఒకవేళ (rhs > 0) {
            Ast_కణుపు_సమాసం* స్థిరరాశి newp
                = కొత్త AstSub{lhsp->fileline(), lhsp,
                             కొత్త AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            తిరిగి newp;
        } లేకపోతే {  // rhs < 0;
            Ast_కణుపు_సమాసం* స్థిరరాశి newp
                = కొత్త Ast_కూడు{lhsp->fileline(), lhsp,
                             కొత్త AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            తిరిగి newp;
        }
    }
    Ast_కణుపు_సమాసం* newSubNeg(int32_t lhs, Ast_కణుపు_సమాసం* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstSub{
            rhsp->fileline(), కొత్త AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        తిరిగి newp;
    }

    Ast_కణుపు_సమాసం* new_Sub_Lsb_యొక్క(Ast_కణుపు_సమాసం* underp, స్థిరరాశి VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        ఒకవేళ (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            తిరిగి underp;
        } లేకపోతే {
            ఒకవేళ (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                తిరిగి newSubNeg(fromRange.hi(), underp);
            } లేకపోతే {
                // reg [3:1] needs a SUB(under,1)
                తిరిగి newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_కణుపు_సమాసం* newMulConst(File_పంక్తి* fl, uint32_t elwidth, Ast_కణుపు_సమాసం* indexp) {
        Ast_కణుపు_సమాసం* extendp;
        స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = std::max(V3_సంఖ్య::log2b(elwidth) + 1, indexp->వెడల్పు_Min());
        ఒకవేళ (indexp->వెడల్పు() > 32) {
            extendp = కొత్త AstSel{fl, indexp, 0, 32, వెడల్పు_Min};
        } లేకపోతే {
            extendp = కొత్త AstExtend{fl, indexp, 32, వెడల్పు_Min};
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి mulp
            = కొత్త AstMul{fl, కొత్త AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        తిరిగి mulp;
    }

    Ast_కణుపు_DType* ముక్క_DType(AstPackArrayDType* nodep, పూర్ణాంకం msb, పూర్ణాంకం lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        ఒకవేళ (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            తిరిగి nodep;
        } లేకపోతే {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            స్థిరరాశి VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_కణుపు_DType* స్థిరరాశి vardtypep
                = కొత్త AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        కొత్త AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(vardtypep);
            తిరిగి vardtypep;
        }
    }

    Ast_కణుపు_సమాసం* sel_వరుస_Backness(Ast_కణుపు* nodep) {
        ఒకవేళ (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            తిరిగి కొత్త AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_కణుపు_సమాసం* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlink_Fr_వెనుక();
            తిరిగి subrhsp;
        } లేకపోతే {
            తిరిగి శూన్యసూచిక;
        }
    }

    శూన్య warnTri(Ast_కణుపు* nodep) {
        ఒకవేళ (VN_IS(nodep, Const) && VN_AS(nodep, Const)->అంకె().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    శూన్య సందర్శించు(Ast_Sel_బిట్టు* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->bitp()->unlink_Fr_వెనుక();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        స్థిరరాశి From_దత్తాంశం fromdata = from_దత్తాంశం_For_Array(nodep, fromp);
        Ast_కణుపు_DType* స్థిరరాశి ddtypep = fromdata.m_dtypep;
        స్థిరరాశి VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_కణుపు_సమాసం* subp = rhsp;
            ఒకవేళ (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* స్థిరరాశి newp = కొత్త AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_కణుపు_సమాసం* subp = rhsp;
            ఒకవేళ (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } లేకపోతే {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->వెడల్పు() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->వెడల్పు() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            స్థిరరాశి పూర్ణాంకం elwidth = adtypep->వెడల్పు() / fromRange.elements();
            AstSel* స్థిరరాశి newp = కొత్త AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->decl_El_వెడల్పు(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            ఒకవేళ (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(అబద్ధం));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstAssocArrayDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* స్థిరరాశి newp = కొత్త AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstWildcardArrayDType* స్థిరరాశి adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* స్థిరరాశి newp = కొత్త AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            స్థిరరాశి VCMethod పద్ధతి
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* స్థిరరాశి newp
                = కొత్త AstCMethodHard{nodep->fileline(), fromp, పద్ధతి, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి adtypep = VN_CAST(ddtypep, వరుస_DType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి backnessp = sel_వరుస_Backness(rhsp)) {
                స్థిరరాశి VCMethod పద్ధతి = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_వెనుక
                                            : VCMethod::ARRAY_AT_వెనుక;
                newp = కొత్త AstCMethodHard{nodep->fileline(), fromp, పద్ధతి, backnessp};
            } లేకపోతే {
                స్థిరరాశి VCMethod పద్ధతి = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = కొత్త AstCMethodHard{nodep->fileline(), fromp, పద్ధతి, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_కణుపు_సమాసం* exprp = fromp;
            అయితే (నిజం) {
                ఒకవేళ (AstMemberSel* స్థిరరాశి memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } లేకపోతే ఒకవేళ (AstStructSel* స్థిరరాశి structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } లేకపోతే {
                    ఆపు;
                }
            }
            స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(exprp, కణుపు_Var_Ref);
            Ast_కణుపు_సమాసం* newp;
            ఒకవేళ (!varrefp || varrefp->access().isReadOnly()) {
                newp = కొత్త AstGetcN{nodep->fileline(), fromp, rhsp};
            } లేకపోతే {
                newp = కొత్త AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* స్థిరరాశి newp
                = కొత్త AstSel{nodep->fileline(), fromp, new_Sub_Lsb_యొక్క(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, కణుపు_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* స్థిరరాశి newp
                = కొత్త AstSel{nodep->fileline(), fromp, new_Sub_Lsb_యొక్క(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
        ఒకవేళ (!rhsp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
    }
    శూన్య సందర్శించు(Ast_Sel_వెలికితీయి* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp()->unlink_Fr_వెనుక();
        స్థిరరాశి From_దత్తాంశం fromdata = from_దత్తాంశం_For_Array(nodep, fromp);
        Ast_కణుపు_DType* స్థిరరాశి ddtypep = fromdata.m_dtypep;
        స్థిరరాశి VNumRange fromRange = fromdata.m_fromRange;
        ఒకవేళ (VN_IS(ddtypep, వరుస_DType)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి qleftp = nodep->leftp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి qrightp = nodep->rightp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి qleftBacknessp = sel_వరుస_Backness(qleftp);
            Ast_కణుపు_సమాసం* స్థిరరాశి qrightBacknessp = sel_వరుస_Backness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            స్థిరరాశి VCMethod పద్ధతి = (qleftBacknessp    ? VCMethod::DYN_ముక్క_వెనుక_వెనుక
                                     : qrightBacknessp ? VCMethod::DYN_ముక్క_FRONT_వెనుక
                                                       : VCMethod::DYN_ముక్క);
            AstCMethodHard* స్థిరరాశి newp = కొత్త AstCMethodHard{nodep->fileline(), fromp, పద్ధతి};
            ఒకవేళ (qleftBacknessp) {
                VL_DO_DANGLING(నెట్టు_Deletep(qleftp), qleftp);
                newp->కూడు_Pinsp(qleftBacknessp);
            } లేకపోతే {
                newp->కూడు_Pinsp(qleftp);
            }
            ఒకవేళ (qrightBacknessp) {
                VL_DO_DANGLING(నెట్టు_Deletep(qrightp), qrightp);
                newp->కూడు_Pinsp(qrightBacknessp);
            } లేకపోతే {
                newp->కూడు_Pinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->did_వెడల్పు(నిజం);
            newp->protect(అబద్ధం);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_కణుపు_సమాసం* స్థిరరాశి msbp = nodep->leftp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = nodep->rightp()->unlink_Fr_వెనుక();
        int32_t msb = VN_AS(msbp, Const)->గమ్యం_SInt();
        int32_t lsb = VN_AS(lsbp, Const)->గమ్యం_SInt();
        స్థిరరాశి int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        ఒకవేళ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            ఒకవేళ (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* స్థిరరాశి newp = కొత్త AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే {  //ముక్క
                Ast_ముక్క_Sel* స్థిరరాశి newp
                    = కొత్త Ast_ముక్క_Sel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        } లేకపోతే ఒకవేళ (AstPackArrayDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->వెడల్పు() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->వెడల్పు() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            ఒకవేళ (fromRange.ascending()) std::మార్పిడి(msb, lsb);
            ఒకవేళ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::మార్పిడి(msb, lsb);
            }
            స్థిరరాశి పూర్ణాంకం elwidth = adtypep->వెడల్పు() / fromRange.elements();
            AstSel* స్థిరరాశి newp
                = కొత్త AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, new_Sub_Lsb_యొక్క(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->decl_El_వెడల్పు(elwidth);
            newp->dtypeFrom(ముక్క_DType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->వెడల్పు_Min() == newp->వెడల్పు_Const(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            ఒకవేళ (fromRange.ascending()) std::మార్పిడి(msb, lsb);
            ఒకవేళ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::మార్పిడి(msb, lsb);
            }
            AstSel* స్థిరరాశి newp = కొత్త AstSel{nodep->fileline(), fromp, new_Sub_Lsb_యొక్క(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, కణుపు_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            ఒకవేళ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::మార్పిడి(msb, lsb);
            }
            AstSel* స్థిరరాశి newp = కొత్త AstSel{nodep->fileline(), fromp, new_Sub_Lsb_యొక్క(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ఒకవేళ (!fromp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(fromp), fromp);
        ఒకవేళ (!msbp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(msbp), msbp);
        ఒకవేళ (!lsbp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(lsbp), lsbp);
    }

    శూన్య replaceSelPlusMinus(Ast_కణుపు_Pre_Sel* nodep) {
        // Select of a range specified with +: or -:, i.e. "array[2+:3], [2-:3]"
        // This select style has a lsb and width
        UINFO(6, "SELPLUS/MINUS " << nodep);
        // Below 2 lines may change nodep->widthp()
        UINFOTREE(9, nodep, "", "SELPM0");
        V3_వెడల్పు::వెడల్పు_Params_Edit(nodep->rhsp());  // constifyEdit doesn't ensure widths finished
        V3Const::constifyEdit(nodep->rhsp());  // May relink pointed to node, ok if not const
        V3Const::constifyParamsEdit(nodep->thsp());  // May relink pointed to node
        checkConstantOrReplace(nodep->thsp(),
                               "Width of :+ or :- bit slice range isn't a constant");
        UINFOTREE(9, nodep, "", "SELPM3");
        // Now replace it with an AstSel
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి widthp = nodep->thsp()->unlink_Fr_వెనుక();
        warnTri(rhsp);
        స్థిరరాశి పూర్ణాంకం వెడల్పు = VN_AS(widthp, Const)->గమ్యం_SInt();
        ఒకవేళ (వెడల్పు > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        ఒకవేళ (వెడల్పు < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        స్థిరరాశి From_దత్తాంశం fromdata = from_దత్తాంశం_For_Array(nodep, fromp);
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి ddtypep = fromdata.m_dtypep;
        స్థిరరాశి VNumRange fromRange = fromdata.m_fromRange;
        ఒకవేళ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            ఒకవేళ (fromRange.elements() == వెడల్పు && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->గమ్యం_SInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* స్థిరరాశి newp = కొత్త AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే ఒకవేళ (VN_IS(rhsp, Const)) {  //ముక్క
                స్థిరరాశి int32_t rhs = VN_AS(rhsp, Const)->గమ్యం_SInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                స్థిరరాశి int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + వెడల్పు - 1 : rhs) - fromRange.lo();
                స్థిరరాశి int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - వెడల్పు + 1) - fromRange.lo();
                Ast_ముక్క_Sel* స్థిరరాశి newp = కొత్త Ast_ముక్క_Sel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } లేకపోతే ఒకవేళ (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, కణుపు_UOr_Struct_DType)
                       && VN_AS(ddtypep, కణుపు_UOr_Struct_DType)->packedUnsup())) {
            పూర్ణాంకం elwidth = 1;
            పూర్ణాంకం newwidth = వెడల్పు;
            ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->వెడల్పు() / fromRange.elements();
                newwidth = వెడల్పు * elwidth;
            }
            Ast_కణుపు_సమాసం* newlsbp = శూన్యసూచిక;
            ఒకవేళ (VN_IS(nodep, SelPlus)) {
                ఒకవేళ (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - వెడల్పు + 1), rhsp);
                } లేకపోతే {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } లేకపోతే ఒకవేళ (VN_IS(nodep, SelMinus)) {
                ఒకవేళ (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } లేకపోతే {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (వెడల్పు - 1));
                }
            } లేకపోతే {
                nodep->v3fatalSrc("Bad Case");
            }
            ఒకవేళ (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* స్థిరరాశి newp = కొత్త AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->decl_El_వెడల్పు(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ఒకవేళ (!fromp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(fromp), fromp);
        ఒకవేళ (!rhsp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
        ఒకవేళ (!widthp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(widthp), widthp);
    }
    శూన్య సందర్శించు(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    శూన్య సందర్శించు(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

బహిరంగ:
    // CONSTRUCTORS
    వెడల్పు_Sel_Visitor() = అప్రమేయ;
    ~వెడల్పు_Sel_Visitor() override = అప్రమేయ;
    Ast_కణుపు* mainAcceptEdit(Ast_కణుపు* nodep) { తిరిగి iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_కణుపు* V3_వెడల్పు::వెడల్పు_Sel_No_Iter_Edit(Ast_కణుపు* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    వెడల్పు_Sel_Visitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    తిరిగి nodep;
}

#నిర్వచనం_రద్దు iterateChildren
