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

ವರ್ಗ ಅಗಲ_Sel_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#ವ್ಯಾಖ್ಯಾನಿಸು iterateChildren DO_NOT_iterate_Childern_ಒಳ_V3_ಅಗಲ_Sel

    // METHODS

    ಶೂನ್ಯ checkConstantOrReplace(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        ಆದರೆ (!VN_IS(nodep, Const)) {
            nodep->v3error(ಸಂದೇಶ);
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    ರಚನೆ From_ದತ್ತಾಂಶ final {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ m_errp;  // Node that was found, for error reporting if not known type
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        From_ದತ್ತಾಂಶ(Ast_ಗಂಟು_DType* errp, Ast_ಗಂಟು_DType* dtypep, ಸ್ಥಿರಾಂಕ VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~From_ದತ್ತಾಂಶ() = ಪೂರ್ವನಿಯೋಜಿತ;
    };
    ಸ್ಥಿರ From_ದತ್ತಾಂಶ from_ದತ್ತಾಂಶ_For_Array(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        ಆಗ (basefromp) {
            ಆದರೆ (VN_IS(basefromp, Attr_ದ)) {
                basefromp = VN_AS(basefromp, Attr_ದ)->fromp();
                ಮುಂದುವರಿ;
            }
            ನಿಲ್ಲಿಸು;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ddtypep = basefromp->dtypep()->skipRefp();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, ಗಂಟು_Array_DType)) {
            fromRange = adtypep->declRange();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, AssocArrayDType)) {
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, WildcardArrayDType)) {
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, DynArrayDType)) {
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, ಸರತಿ_DType)) {
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ adtypep
                   = VN_CAST(ddtypep, ಗಂಟು_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, BasicDType)) {
            ಆದರೆ (adtypep->isString() && VN_IS(nodep, Sel_ಬಿಟ್)) {
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        ಹಿಂತಿರುಗಿ From_ದತ್ತಾಂಶ(errp, ddtypep, fromRange);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSubNeg(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        ಆದರೆ (rhs == 0) {
            ಹಿಂತಿರುಗಿ lhsp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ(lhsp, lhsp->ಅಗಲ());
            ಅಂಕೆ.opSub(VN_AS(lhsp, Const)->ಅಂಕೆ(), V3_ಸಂಖ್ಯೆ(lhsp, 32, rhs));
            ಅಂಕೆ.isSigned(lhsp->isSigned());
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{lhsp->fileline(), ಅಂಕೆ};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhs > 0) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstSub{lhsp->fileline(), lhsp,
                             ಹೊಸ AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ಹಿಂತಿರುಗಿ newp;
        } ಇಲ್ಲದಿದ್ದರೆ {  // rhs < 0;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ Ast_ಕೂಡಿಸು{lhsp->fileline(), lhsp,
                             ಹೊಸ AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ಹಿಂತಿರುಗಿ newp;
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSubNeg(int32_t lhs, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSub{
            rhsp->fileline(), ಹೊಸ AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        ಹಿಂತಿರುಗಿ newp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Sub_Lsb_ದ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* underp, ಸ್ಥಿರಾಂಕ VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        ಆದರೆ (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            ಹಿಂತಿರುಗಿ underp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                ಹಿಂತಿರುಗಿ newSubNeg(fromRange.hi(), underp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // reg [3:1] needs a SUB(under,1)
                ಹಿಂತಿರುಗಿ newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newMulConst(File_ಸಾಲು* fl, uint32_t elwidth, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* extendp;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = std::max(V3_ಸಂಖ್ಯೆ::log2b(elwidth) + 1, indexp->ಅಗಲ_Min());
        ಆದರೆ (indexp->ಅಗಲ() > 32) {
            extendp = ಹೊಸ AstSel{fl, indexp, 0, 32, ಅಗಲ_Min};
        } ಇಲ್ಲದಿದ್ದರೆ {
            extendp = ಹೊಸ AstExtend{fl, indexp, 32, ಅಗಲ_Min};
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ mulp
            = ಹೊಸ AstMul{fl, ಹೊಸ AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        ಹಿಂತಿರುಗಿ mulp;
    }

    Ast_ಗಂಟು_DType* ತುಂಡು_DType(AstPackArrayDType* nodep, ಪೂರ್ಣಾಂಕ msb, ಪೂರ್ಣಾಂಕ lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        ಆದರೆ (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            ಹಿಂತಿರುಗಿ nodep;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ಸ್ಥಿರಾಂಕ VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ vardtypep
                = ಹೊಸ AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        ಹೊಸ AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(vardtypep);
            ಹಿಂತಿರುಗಿ vardtypep;
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sel_ಸರತಿ_Backness(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಹಿಂತಿರುಗಿ subrhsp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
    }

    ಶೂನ್ಯ warnTri(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (VN_IS(nodep, Const) && VN_AS(nodep, Const)->ಅಂಕೆ().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sel_ಬಿಟ್* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ಸ್ಥಿರಾಂಕ From_ದತ್ತಾಂಶ fromdata = from_ದತ್ತಾಂಶ_For_Array(nodep, fromp);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ddtypep = fromdata.m_dtypep;
        ಸ್ಥಿರಾಂಕ VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subp = rhsp;
            ಆದರೆ (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subp = rhsp;
            ಆದರೆ (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->ಅಗಲ() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->ಅಗಲ() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elwidth = adtypep->ಅಗಲ() / fromRange.elements();
            AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->decl_El_ಅಗಲ(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            ಆದರೆ (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(ಸುಳ್ಳು));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstWildcardArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, ವಿಧಾನ, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, ಸರತಿ_DType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ backnessp = sel_ಸರತಿ_Backness(rhsp)) {
                ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_ಹಿಂಭಾಗ
                                            : VCMethod::ARRAY_AT_ಹಿಂಭಾಗ;
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, ವಿಧಾನ, backnessp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, ವಿಧಾನ, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = fromp;
            ಆಗ (ನಿಜ) {
                ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstStructSel* ಸ್ಥಿರಾಂಕ structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(exprp, ಗಂಟು_Var_Ref);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಆದರೆ (!varrefp || varrefp->access().isReadOnly()) {
                newp = ಹೊಸ AstGetcN{nodep->fileline(), fromp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp = ಹೊಸ AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ದ(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, ಗಂಟು_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ದ(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        ಆದರೆ (!rhsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sel_ಹೊರತೆಗೆ* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ From_ದತ್ತಾಂಶ fromdata = from_ದತ್ತಾಂಶ_For_Array(nodep, fromp);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ddtypep = fromdata.m_dtypep;
        ಸ್ಥಿರಾಂಕ VNumRange fromRange = fromdata.m_fromRange;
        ಆದರೆ (VN_IS(ddtypep, ಸರತಿ_DType)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ qleftp = nodep->leftp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ qrightp = nodep->rightp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ qleftBacknessp = sel_ಸರತಿ_Backness(qleftp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ qrightBacknessp = sel_ಸರತಿ_Backness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ = (qleftBacknessp    ? VCMethod::DYN_ತುಂಡು_ಹಿಂಭಾಗ_ಹಿಂಭಾಗ
                                     : qrightBacknessp ? VCMethod::DYN_ತುಂಡು_FRONT_ಹಿಂಭಾಗ
                                                       : VCMethod::DYN_ತುಂಡು);
            AstCMethodHard* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCMethodHard{nodep->fileline(), fromp, ವಿಧಾನ};
            ಆದರೆ (qleftBacknessp) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(qleftp), qleftp);
                newp->ಕೂಡಿಸು_Pinsp(qleftBacknessp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp->ಕೂಡಿಸು_Pinsp(qleftp);
            }
            ಆದರೆ (qrightBacknessp) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(qrightp), qrightp);
                newp->ಕೂಡಿಸು_Pinsp(qrightBacknessp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                newp->ಕೂಡಿಸು_Pinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->did_ಅಗಲ(ನಿಜ);
            newp->protect(ಸುಳ್ಳು);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ msbp = nodep->leftp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsbp = nodep->rightp()->unlink_Fr_ಹಿಂಭಾಗ();
        int32_t msb = VN_AS(msbp, Const)->ಗಮ್ಯ_SInt();
        int32_t lsb = VN_AS(lsbp, Const)->ಗಮ್ಯ_SInt();
        ಸ್ಥಿರಾಂಕ int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        ಆದರೆ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            ಆದರೆ (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {  //ತುಂಡು
                Ast_ತುಂಡು_Sel* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ Ast_ತುಂಡು_Sel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstPackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->ಅಗಲ() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->ಅಗಲ() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            ಆದರೆ (fromRange.ascending()) std::ಅದಲುಬದಲು(msb, lsb);
            ಆದರೆ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::ಅದಲುಬದಲು(msb, lsb);
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elwidth = adtypep->ಅಗಲ() / fromRange.elements();
            AstSel* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, new_Sub_Lsb_ದ(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->decl_El_ಅಗಲ(elwidth);
            newp->dtypeFrom(ತುಂಡು_DType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->ಅಗಲ_Min() == newp->ಅಗಲ_Const(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            ಆದರೆ (fromRange.ascending()) std::ಅದಲುಬದಲು(msb, lsb);
            ಆದರೆ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::ಅದಲುಬದಲು(msb, lsb);
            }
            AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ದ(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, ಗಂಟು_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            ಆದರೆ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::ಅದಲುಬದಲು(msb, lsb);
            }
            AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ದ(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ಆದರೆ (!fromp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        ಆದರೆ (!msbp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(msbp), msbp);
        ಆದರೆ (!lsbp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(lsbp), lsbp);
    }

    ಶೂನ್ಯ replaceSelPlusMinus(Ast_ಗಂಟು_Pre_Sel* nodep) {
        // Select of a range specified with +: or -:, i.e. "array[2+:3], [2-:3]"
        // This select style has a lsb and width
        UINFO(6, "SELPLUS/MINUS " << nodep);
        // Below 2 lines may change nodep->widthp()
        UINFOTREE(9, nodep, "", "SELPM0");
        V3_ಅಗಲ::ಅಗಲ_Params_Edit(nodep->rhsp());  // constifyEdit doesn't ensure widths finished
        V3Const::constifyEdit(nodep->rhsp());  // May relink pointed to node, ok if not const
        V3Const::constifyParamsEdit(nodep->thsp());  // May relink pointed to node
        checkConstantOrReplace(nodep->thsp(),
                               "Width of :+ or :- bit slice range isn't a constant");
        UINFOTREE(9, nodep, "", "SELPM3");
        // Now replace it with an AstSel
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ widthp = nodep->thsp()->unlink_Fr_ಹಿಂಭಾಗ();
        warnTri(rhsp);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = VN_AS(widthp, Const)->ಗಮ್ಯ_SInt();
        ಆದರೆ (ಅಗಲ > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        ಆದರೆ (ಅಗಲ < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ಸ್ಥಿರಾಂಕ From_ದತ್ತಾಂಶ fromdata = from_ದತ್ತಾಂಶ_For_Array(nodep, fromp);
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ddtypep = fromdata.m_dtypep;
        ಸ್ಥಿರಾಂಕ VNumRange fromRange = fromdata.m_fromRange;
        ಆದರೆ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            ಆದರೆ (fromRange.elements() == ಅಗಲ && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->ಗಮ್ಯ_SInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(rhsp, Const)) {  //ತುಂಡು
                ಸ್ಥಿರಾಂಕ int32_t rhs = VN_AS(rhsp, Const)->ಗಮ್ಯ_SInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ಸ್ಥಿರಾಂಕ int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + ಅಗಲ - 1 : rhs) - fromRange.lo();
                ಸ್ಥಿರಾಂಕ int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - ಅಗಲ + 1) - fromRange.lo();
                Ast_ತುಂಡು_Sel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ತುಂಡು_Sel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, ಗಂಟು_UOr_Struct_DType)
                       && VN_AS(ddtypep, ಗಂಟು_UOr_Struct_DType)->packedUnsup())) {
            ಪೂರ್ಣಾಂಕ elwidth = 1;
            ಪೂರ್ಣಾಂಕ newwidth = ಅಗಲ;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->ಅಗಲ() / fromRange.elements();
                newwidth = ಅಗಲ * elwidth;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newlsbp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (VN_IS(nodep, SelPlus)) {
                ಆದರೆ (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - ಅಗಲ + 1), rhsp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, SelMinus)) {
                ಆದರೆ (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (ಅಗಲ - 1));
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Bad Case");
            }
            ಆದರೆ (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->decl_El_ಅಗಲ(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ಆದರೆ (!fromp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
        ಆದರೆ (!rhsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
        ಆದರೆ (!widthp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(widthp), widthp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಅಗಲ_Sel_Visitor() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~ಅಗಲ_Sel_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_ಗಂಟು* V3_ಅಗಲ::ಅಗಲ_Sel_No_Iter_Edit(Ast_ಗಂಟು* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    ಅಗಲ_Sel_Visitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    ಹಿಂತಿರುಗಿ nodep;
}

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು iterateChildren
