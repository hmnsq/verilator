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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

വർഗ്ഗം WidthSelVisitor final : പൊതു VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#define iterateChildren DO_NOT_iterateChildern_IN_V3WidthSel

    // METHODS

    ശൂന്യം checkConstantOrReplace(Ast_കണ്ണി* nodep, സ്ഥിരം string& message) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        എങ്കിൽ (!VN_IS(nodep, Const)) {
            nodep->v3error(message);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    ഘടന FromData final {
        Ast_കണ്ണി_DType* സ്ഥിരം m_errp;  // Node that was found, for error reporting if not known type
        Ast_കണ്ണി_DType* സ്ഥിരം m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        FromData(Ast_കണ്ണി_DType* errp, Ast_കണ്ണി_DType* dtypep, സ്ഥിരം VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~FromData() = സ്ഥിരസ്ഥിതി;
    };
    സ്ഥിതം FromData fromDataForArray(Ast_കണ്ണി* nodep, Ast_കണ്ണി* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        ആകുമ്പോൾ (basefromp) {
            എങ്കിൽ (VN_IS(basefromp, AttrOf)) {
                basefromp = VN_AS(basefromp, AttrOf)->fromp();
                തുടരുക;
            }
            നിർത്തുക;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_കണ്ണി_DType* സ്ഥിരം ddtypep = basefromp->dtypep()->skipRefp();
        Ast_കണ്ണി_DType* സ്ഥിരം errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(ddtypep, കണ്ണി_Array_DType)) {
            fromRange = adtypep->declRange();
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, AssocArrayDType)) {
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, WildcardArrayDType)) {
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, DynArrayDType)) {
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, QueueDType)) {
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം adtypep
                   = VN_CAST(ddtypep, കണ്ണി_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, BasicDType)) {
            എങ്കിൽ (adtypep->isString() && VN_IS(nodep, SelBit)) {
            } അല്ലെങ്കിൽ എങ്കിൽ (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } അല്ലെങ്കിൽ {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } അല്ലെങ്കിൽ {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        തിരികെ FromData(errp, ddtypep, fromRange);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newSubNeg(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        എങ്കിൽ (rhs == 0) {
            തിരികെ lhsp;
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_സംഖ്യ അക്കം(lhsp, lhsp->width());
            അക്കം.opSub(VN_AS(lhsp, Const)->അക്കം(), V3_സംഖ്യ(lhsp, 32, rhs));
            അക്കം.isSigned(lhsp->isSigned());
            തിരികെ പുതിയ AstConst{lhsp->fileline(), അക്കം};
        } അല്ലെങ്കിൽ എങ്കിൽ (rhs > 0) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                = പുതിയ AstSub{lhsp->fileline(), lhsp,
                             പുതിയ AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            തിരികെ newp;
        } അല്ലെങ്കിൽ {  // rhs < 0;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                = പുതിയ AstAdd{lhsp->fileline(), lhsp,
                             പുതിയ AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            തിരികെ newp;
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* newSubNeg(int32_t lhs, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstSub{
            rhsp->fileline(), പുതിയ AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        തിരികെ newp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newSubLsbOf(Ast_കണ്ണി_വ്യഞ്ജകം* underp, സ്ഥിരം VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        എങ്കിൽ (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            തിരികെ underp;
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                തിരികെ newSubNeg(fromRange.hi(), underp);
            } അല്ലെങ്കിൽ {
                // reg [3:1] needs a SUB(under,1)
                തിരികെ newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* newMulConst(File_വരി* fl, uint32_t elwidth, Ast_കണ്ണി_വ്യഞ്ജകം* indexp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* extendp;
        സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = std::max(V3_സംഖ്യ::log2b(elwidth) + 1, indexp->widthMin());
        എങ്കിൽ (indexp->width() > 32) {
            extendp = പുതിയ AstSel{fl, indexp, 0, 32, widthMin};
        } അല്ലെങ്കിൽ {
            extendp = പുതിയ AstExtend{fl, indexp, 32, widthMin};
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം mulp
            = പുതിയ AstMul{fl, പുതിയ AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        തിരികെ mulp;
    }

    Ast_കണ്ണി_DType* sliceDType(AstPackArrayDType* nodep, പൂർണ്ണസംഖ്യ msb, പൂർണ്ണസംഖ്യ lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        എങ്കിൽ (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            തിരികെ nodep;
        } അല്ലെങ്കിൽ {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            സ്ഥിരം VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_കണ്ണി_DType* സ്ഥിരം vardtypep
                = പുതിയ AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        പുതിയ AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
            തിരികെ vardtypep;
        }
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* selQueueBackness(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            തിരികെ പുതിയ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_കണ്ണി_വ്യഞ്ജകം* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlinkFrBack();
            തിരികെ subrhsp;
        } അല്ലെങ്കിൽ {
            തിരികെ nullptr;
        }
    }

    ശൂന്യം warnTri(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (VN_IS(nodep, Const) && VN_AS(nodep, Const)->അക്കം().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    ശൂന്യം visit(AstSelBit* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->bitp()->unlinkFrBack();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        സ്ഥിരം FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_കണ്ണി_DType* സ്ഥിരം ddtypep = fromdata.m_dtypep;
        സ്ഥിരം VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_കണ്ണി_വ്യഞ്ജകം* subp = rhsp;
            എങ്കിൽ (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* സ്ഥിരം newp = പുതിയ AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_കണ്ണി_വ്യഞ്ജകം* subp = rhsp;
            എങ്കിൽ (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } അല്ലെങ്കിൽ {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            സ്ഥിരം പൂർണ്ണസംഖ്യ elwidth = adtypep->width() / fromRange.elements();
            AstSel* സ്ഥിരം newp = പുതിയ AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            എങ്കിൽ (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(അസത്യം));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstAssocArrayDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* സ്ഥിരം newp = പുതിയ AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstWildcardArrayDType* സ്ഥിരം adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* സ്ഥിരം newp = പുതിയ AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            സ്ഥിരം VCMethod method
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* സ്ഥിരം newp
                = പുതിയ AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstQueueDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, QueueDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം backnessp = selQueueBackness(rhsp)) {
                സ്ഥിരം VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_BACK
                                            : VCMethod::ARRAY_AT_BACK;
                newp = പുതിയ AstCMethodHard{nodep->fileline(), fromp, method, backnessp};
            } അല്ലെങ്കിൽ {
                സ്ഥിരം VCMethod method = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = പുതിയ AstCMethodHard{nodep->fileline(), fromp, method, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_കണ്ണി_വ്യഞ്ജകം* exprp = fromp;
            ആകുമ്പോൾ (സത്യം) {
                എങ്കിൽ (AstMemberSel* സ്ഥിരം memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } അല്ലെങ്കിൽ എങ്കിൽ (AstStructSel* സ്ഥിരം structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } അല്ലെങ്കിൽ {
                    നിർത്തുക;
                }
            }
            സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(exprp, കണ്ണി_Var_Ref);
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            എങ്കിൽ (!varrefp || varrefp->access().isReadOnly()) {
                newp = പുതിയ AstGetcN{nodep->fileline(), fromp, rhsp};
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* സ്ഥിരം newp
                = പുതിയ AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, കണ്ണി_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* സ്ഥിരം newp
                = പുതിയ AstSel{nodep->fileline(), fromp, newSubLsbOf(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
        എങ്കിൽ (!rhsp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
    }
    ശൂന്യം visit(AstSelExtract* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp()->unlinkFrBack();
        സ്ഥിരം FromData fromdata = fromDataForArray(nodep, fromp);
        Ast_കണ്ണി_DType* സ്ഥിരം ddtypep = fromdata.m_dtypep;
        സ്ഥിരം VNumRange fromRange = fromdata.m_fromRange;
        എങ്കിൽ (VN_IS(ddtypep, QueueDType)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം qleftp = nodep->leftp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം qrightp = nodep->rightp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം qleftBacknessp = selQueueBackness(qleftp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം qrightBacknessp = selQueueBackness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            സ്ഥിരം VCMethod method = (qleftBacknessp    ? VCMethod::DYN_SLICE_BACK_BACK
                                     : qrightBacknessp ? VCMethod::DYN_SLICE_FRONT_BACK
                                                       : VCMethod::DYN_SLICE);
            AstCMethodHard* സ്ഥിരം newp = പുതിയ AstCMethodHard{nodep->fileline(), fromp, method};
            എങ്കിൽ (qleftBacknessp) {
                VL_DO_DANGLING(തള്ളുക_Deletep(qleftp), qleftp);
                newp->addPinsp(qleftBacknessp);
            } അല്ലെങ്കിൽ {
                newp->addPinsp(qleftp);
            }
            എങ്കിൽ (qrightBacknessp) {
                VL_DO_DANGLING(തള്ളുക_Deletep(qrightp), qrightp);
                newp->addPinsp(qrightBacknessp);
            } അല്ലെങ്കിൽ {
                newp->addPinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->didWidth(സത്യം);
            newp->protect(അസത്യം);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം msbp = nodep->leftp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = nodep->rightp()->unlinkFrBack();
        int32_t msb = VN_AS(msbp, Const)->toSInt();
        int32_t lsb = VN_AS(lsbp, Const)->toSInt();
        സ്ഥിരം int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        എങ്കിൽ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            എങ്കിൽ (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* സ്ഥിരം newp = പുതിയ AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ {  // Slice
                AstSliceSel* സ്ഥിരം newp
                    = പുതിയ AstSliceSel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstPackArrayDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->width() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->width() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            എങ്കിൽ (fromRange.ascending()) std::swap(msb, lsb);
            എങ്കിൽ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ elwidth = adtypep->width() / fromRange.elements();
            AstSel* സ്ഥിരം newp
                = പുതിയ AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, newSubLsbOf(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            newp->dtypeFrom(sliceDType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->widthMin() == newp->widthConst(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            എങ്കിൽ (fromRange.ascending()) std::swap(msb, lsb);
            എങ്കിൽ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* സ്ഥിരം newp = പുതിയ AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, കണ്ണി_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            എങ്കിൽ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::swap(msb, lsb);
            }
            AstSel* സ്ഥിരം newp = പുതിയ AstSel{nodep->fileline(), fromp, newSubLsbOf(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        എങ്കിൽ (!fromp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(fromp), fromp);
        എങ്കിൽ (!msbp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(msbp), msbp);
        എങ്കിൽ (!lsbp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(lsbp), lsbp);
    }

    ശൂന്യം replaceSelPlusMinus(Ast_കണ്ണി_Pre_Sel* nodep) {
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
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം widthp = nodep->thsp()->unlinkFrBack();
        warnTri(rhsp);
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = VN_AS(widthp, Const)->toSInt();
        എങ്കിൽ (width > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        എങ്കിൽ (width < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        സ്ഥിരം FromData fromdata = fromDataForArray(nodep, fromp);
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം ddtypep = fromdata.m_dtypep;
        സ്ഥിരം VNumRange fromRange = fromdata.m_fromRange;
        എങ്കിൽ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            എങ്കിൽ (fromRange.elements() == width && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->toSInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* സ്ഥിരം newp = പുതിയ AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(rhsp, Const)) {  // Slice
                സ്ഥിരം int32_t rhs = VN_AS(rhsp, Const)->toSInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                സ്ഥിരം int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + width - 1 : rhs) - fromRange.lo();
                സ്ഥിരം int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - width + 1) - fromRange.lo();
                AstSliceSel* സ്ഥിരം newp = പുതിയ AstSliceSel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, കണ്ണി_UOr_Struct_DType)
                       && VN_AS(ddtypep, കണ്ണി_UOr_Struct_DType)->packedUnsup())) {
            പൂർണ്ണസംഖ്യ elwidth = 1;
            പൂർണ്ണസംഖ്യ newwidth = width;
            എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->width() / fromRange.elements();
                newwidth = width * elwidth;
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* newlsbp = nullptr;
            എങ്കിൽ (VN_IS(nodep, SelPlus)) {
                എങ്കിൽ (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - width + 1), rhsp);
                } അല്ലെങ്കിൽ {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, SelMinus)) {
                എങ്കിൽ (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } അല്ലെങ്കിൽ {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (width - 1));
                }
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Bad Case");
            }
            എങ്കിൽ (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* സ്ഥിരം newp = പുതിയ AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->declElWidth(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        എങ്കിൽ (!fromp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(fromp), fromp);
        എങ്കിൽ (!rhsp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
        എങ്കിൽ (!widthp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(widthp), widthp);
    }
    ശൂന്യം visit(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    ശൂന്യം visit(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

പൊതു:
    // CONSTRUCTORS
    WidthSelVisitor() = സ്ഥിരസ്ഥിതി;
    ~WidthSelVisitor() override = സ്ഥിരസ്ഥിതി;
    Ast_കണ്ണി* mainAcceptEdit(Ast_കണ്ണി* nodep) { തിരികെ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_കണ്ണി* V3Width::widthSelNoIterEdit(Ast_കണ്ണി* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthSelVisitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    തിരികെ nodep;
}

#undef iterateChildren
