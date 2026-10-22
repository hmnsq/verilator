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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Width state, as a visitor of each AstNode

ਵਰਗ ਚੌੜਾਈ_Sel_Visitor final : ਜਨਤਕ VNVisitor {
    // IMPORTANT
    //**** This is not a normal visitor, in that all iteration is instead
    //  done by the caller (V3Width).  This avoids duplicating much of the
    //  complicated GenCase/GenFor/Cell/Function call logic that all depends
    //  on if widthing top-down or just for parameters.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ iterateChildren DO_NOT_iterate_Childern_ਵਿੱਚ_V3_ਚੌੜਾਈ_Sel

    // METHODS

    ਖਾਲੀ checkConstantOrReplace(Ast_ਗੰਢ* nodep, ਅਚਲ string& ਸੁਨੇਹਾ) {
        // See also V3Width::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        ਜੇ (!VN_IS(nodep, Const)) {
            nodep->v3error(ਸੁਨੇਹਾ);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    // RETURN TYPE
    ਬਣਤਰ From_ਅੰਕੜੇ final {
        Ast_ਗੰਢ_DType* ਅਚਲ m_errp;  // Node that was found, for error reporting if not known type
        Ast_ਗੰਢ_DType* ਅਚਲ m_dtypep;  // Data type for the 'from' slice
        VNumRange m_fromRange;  // Numeric range bounds for the 'from' slice
        From_ਅੰਕੜੇ(Ast_ਗੰਢ_DType* errp, Ast_ਗੰਢ_DType* dtypep, ਅਚਲ VNumRange& fromRange)
            : m_errp{errp}
            , m_dtypep{dtypep}
            , m_fromRange{fromRange} {}
        ~From_ਅੰਕੜੇ() = ਮੂਲ;
    };
    ਸਥਿਰ From_ਅੰਕੜੇ from_ਅੰਕੜੇ_For_Array(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* basefromp) {
        // What is the data type and information for this SEL-ish's from()?
        UINFO(9, "  fromData start ddtypep = " << basefromp);
        VNumRange fromRange;  // constructs to isRanged(false)
        ਜਦੋਂ (basefromp) {
            ਜੇ (VN_IS(basefromp, Attr_ਦਾ)) {
                basefromp = VN_AS(basefromp, Attr_ਦਾ)->fromp();
                ਜਾਰੀ;
            }
            ਰੋਕੋ;
        }
        UASSERT_OBJ(basefromp && basefromp->dtypep(), nodep, "Select with no from dtype");
        Ast_ਗੰਢ_DType* ਅਚਲ ddtypep = basefromp->dtypep()->skipRefp();
        Ast_ਗੰਢ_DType* ਅਚਲ errp = ddtypep;
        UINFO(9, "  fromData.ddtypep = " << ddtypep);
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(ddtypep, ਗੰਢ_Array_DType)) {
            fromRange = adtypep->declRange();
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, AssocArrayDType)) {
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, WildcardArrayDType)) {
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, DynArrayDType)) {
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, ਕਤਾਰ_DType)) {
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ adtypep
                   = VN_CAST(ddtypep, ਗੰਢ_UOr_Struct_DType)) {
            fromRange = adtypep->declRange();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ adtypep = VN_CAST(ddtypep, BasicDType)) {
            ਜੇ (adtypep->isString() && VN_IS(nodep, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ)) {
            } ਨਹੀਂਤਾਂ ਜੇ (adtypep->isRanged()) {
                UASSERT_OBJ(
                    !(adtypep->rangep()
                      && (!VN_IS(adtypep->rangep()->leftp(), Const)
                          || !VN_IS(adtypep->rangep()->rightp(), Const))),
                    nodep,
                    "Non-constant variable range; errored earlier");  // in constifyParam(bfdtypep)
                fromRange = adtypep->declRange();
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Illegal bit or array select; type does not have a bit range, or "
                               << "bad dimension: data type is " << errp->prettyDTypeNameQ());
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is " << errp->prettyDTypeNameQ());
        }
        ਵਾਪਸ From_ਅੰਕੜੇ(errp, ddtypep, fromRange);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newSubNeg(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, int32_t rhs) {
        // Return lhs-rhs, but if rhs is negative use an add, so we won't
        // have to deal with signed math and related 32bit sign extension problems
        ਜੇ (rhs == 0) {
            ਵਾਪਸ lhsp;
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(lhsp, Const)) {
            // Optional vs just making add/sub below, but saves constification some work
            V3_ਸੰਖਿਆ ਅੰਕ(lhsp, lhsp->ਚੌੜਾਈ());
            ਅੰਕ.opSub(VN_AS(lhsp, Const)->ਅੰਕ(), V3_ਸੰਖਿਆ(lhsp, 32, rhs));
            ਅੰਕ.isSigned(lhsp->isSigned());
            ਵਾਪਸ ਨਵਾਂ AstConst{lhsp->fileline(), ਅੰਕ};
        } ਨਹੀਂਤਾਂ ਜੇ (rhs > 0) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                = ਨਵਾਂ AstSub{lhsp->fileline(), lhsp,
                             ਨਵਾਂ AstConst(lhsp->fileline(), AstConst::Unsized32{}, rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ਵਾਪਸ newp;
        } ਨਹੀਂਤਾਂ {  // rhs < 0;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                = ਨਵਾਂ Ast_ਜੋੜੋ{lhsp->fileline(), lhsp,
                             ਨਵਾਂ AstConst(lhsp->fileline(), AstConst::Unsized32{}, -rhs)};
            // We must make sure sub gets sign of original value, not from the constant
            newp->dtypeFrom(lhsp);
            ਵਾਪਸ newp;
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newSubNeg(int32_t lhs, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        // Return lhs-rhs
        // We must make sure sub gets sign of original value
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstSub{
            rhsp->fileline(), ਨਵਾਂ AstConst(rhsp->fileline(), AstConst::Unsized32{}, lhs), rhsp};
        newp->dtypeFrom(rhsp);  // Important as AstSub default is lhs's sign
        ਵਾਪਸ newp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Sub_Lsb_ਦਾ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* underp, ਅਚਲ VNumRange& fromRange) {
        // Account for a variable's LSB in bit selections
        // Will likely become SUB(underp, lsb_of_signal).
        // Don't report WIDTH warnings etc here, as may be inside a
        // generate branch that will be deleted.
        // SUB #'s Not needed when LSB==0 and MSB>=0 (ie [0:-13] must still get added!)
        ਜੇ (!fromRange.ranged()) {
            // vector without range, or 0 lsb is ok, for example a INTEGER x; y = x[21:0];
            ਵਾਪਸ underp;
        } ਨਹੀਂਤਾਂ {
            ਜੇ (fromRange.ascending()) {
                // reg [1:3] was swapped to [3:1] (lsbEndianedp==3) and needs a SUB(3,under)
                ਵਾਪਸ newSubNeg(fromRange.hi(), underp);
            } ਨਹੀਂਤਾਂ {
                // reg [3:1] needs a SUB(under,1)
                ਵਾਪਸ newSubNeg(underp, fromRange.lo());
            }
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newMulConst(File_ਸਤਰ* fl, uint32_t elwidth, Ast_ਗੰਢ_ਵਿਅੰਜਕ* indexp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* extendp;
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = std::max(V3_ਸੰਖਿਆ::log2b(elwidth) + 1, indexp->ਚੌੜਾਈ_Min());
        ਜੇ (indexp->ਚੌੜਾਈ() > 32) {
            extendp = ਨਵਾਂ AstSel{fl, indexp, 0, 32, ਚੌੜਾਈ_Min};
        } ਨਹੀਂਤਾਂ {
            extendp = ਨਵਾਂ AstExtend{fl, indexp, 32, ਚੌੜਾਈ_Min};
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ mulp
            = ਨਵਾਂ AstMul{fl, ਨਵਾਂ AstConst{fl, AstConst::Unsized32{}, elwidth},
                         // Extend needed as index might be e.g. 3 bits but constant e.g. 5 bits
                         extendp};
        ਵਾਪਸ mulp;
    }

    Ast_ਗੰਢ_DType* ਟੁਕੜਾ_DType(AstPackArrayDType* nodep, ਪੂਰਨ msb, ਪੂਰਨ lsb) {
        // Return slice needed for msb/lsb, either as original dtype or a new slice dtype
        ਜੇ (nodep->declRange().elements() == (msb - lsb + 1)  // Extracting whole of original array
            && nodep->declRange().lo() == lsb) {
            ਵਾਪਸ nodep;
        } ਨਹੀਂਤਾਂ {
            // Need a slice data type, which is an array of the extracted
            // type, but with (presumably) different size
            ਅਚਲ VNumRange newRange{msb, lsb, nodep->declRange().ascending()};
            Ast_ਗੰਢ_DType* ਅਚਲ vardtypep
                = ਨਵਾਂ AstPackArrayDType{nodep->fileline(),
                                        nodep->subDTypep(),  // Need to strip off array reference
                                        ਨਵਾਂ AstRange{nodep->fileline(), newRange}};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(vardtypep);
            ਵਾਪਸ vardtypep;
        }
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* sel_ਕਤਾਰ_Backness(Ast_ਗੰਢ* nodep) {
        ਜੇ (VN_IS(nodep, Unbounded)) {  // e.g. "[$]"
            ਵਾਪਸ ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Signed32{}, 0};
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Sub) && VN_IS(VN_CAST(nodep, Sub)->lhsp(), Unbounded)) {
            // e.g. "q[$ - 1]", where 1 is subnodep
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* subrhsp = VN_CAST(nodep, Sub)->rhsp()->unlink_Fr_ਪਿੱਠ();
            ਵਾਪਸ subrhsp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
    }

    ਖਾਲੀ warnTri(Ast_ਗੰਢ* nodep) {
        ਜੇ (VN_IS(nodep, Const) && VN_AS(nodep, Const)->ਅੰਕ().isFourState()) {
            nodep->v3error(
                "Selection index is constantly unknown or tristated: " << nodep->name());
        }
    }

    // VISITORS
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    ਖਾਲੀ ਦੇਖੋ(Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* nodep) override {
        // Select of a non-width specified part of an array, i.e. "array[2]"
        // This select style has a lsb and msb (no user specified width)
        UINFO(6, "SELBIT " << nodep);
        UINFOTREE(9, nodep->backp(), "", "SELBT0");
        // lhsp/rhsp do not need to be constant
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->bitp()->unlink_Fr_ਪਿੱਠ();  // bit we're extracting
        UINFOTREE(9, nodep, "", "SELBT2");
        ਅਚਲ From_ਅੰਕੜੇ fromdata = from_ਅੰਕੜੇ_For_Array(nodep, fromp);
        Ast_ਗੰਢ_DType* ਅਚਲ ddtypep = fromdata.m_dtypep;
        ਅਚਲ VNumRange fromRange = fromdata.m_fromRange;
        UINFO(6, "  ddtypep " << ddtypep);
        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(ddtypep, UnpackArrayDType)) {
            // SELBIT(array, index) -> ARRAYSEL(array, index)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* subp = rhsp;
            ਜੇ (fromRange.lo() != 0 || fromRange.hi() < 0) {
                subp = newSubNeg(subp, fromRange.lo());
            }
            AstArraySel* ਅਚਲ newp = ਨਵਾਂ AstArraySel{nodep->fileline(), fromp, subp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELBIT(array, index) -> SEL(array, index*width-of-subindex, width-of-subindex)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* subp = rhsp;
            ਜੇ (fromRange.ascending()) {
                subp = newSubNeg(fromRange.hi(), subp);
            } ਨਹੀਂਤਾਂ {
                subp = newSubNeg(subp, fromRange.lo());
            }
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->ਚੌੜਾਈ() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->ਚੌੜਾਈ() << "/"
                                                                   << fromRange.elements());
            // cppcheck-suppress zerodivcond
            ਅਚਲ ਪੂਰਨ elwidth = adtypep->ਚੌੜਾਈ() / fromRange.elements();
            AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{
                nodep->fileline(), fromp, newMulConst(nodep->fileline(), elwidth, subp), elwidth};
            newp->declRange(fromRange);
            newp->decl_El_ਚੌੜਾਈ(elwidth);
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            ਜੇ (VN_IS(adtypep->subDTypep(), BasicDType)) {
                // IEEE 1800-2023 7.4.1 Packed arrays says:
                //   The individual elements of the array are unsigned
                //   unless they are of a named type declared as signed.
                newp->dtypep()->numeric(VSigning::fromBool(ਝੂਠ));
            }
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstAssocArrayDType* ਅਚਲ adtypep = VN_CAST(ddtypep, AssocArrayDType)) {
            // SELBIT(array, index) -> ASSOCSEL(array, index)
            AstAssocSel* ਅਚਲ newp = ਨਵਾਂ AstAssocSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstWildcardArrayDType* ਅਚਲ adtypep
                   = VN_CAST(ddtypep, WildcardArrayDType)) {
            // SELBIT(array, index) -> WILDCARDSEL(array, index)
            AstWildcardSel* ਅਚਲ newp = ਨਵਾਂ AstWildcardSel{nodep->fileline(), fromp, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ adtypep = VN_CAST(ddtypep, DynArrayDType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            ਅਚਲ VCMethod ਵਿਧੀ
                = nodep->access().isWriteOrRW() ? VCMethod::ARRAY_AT_WRITE : VCMethod::ARRAY_AT;
            AstCMethodHard* ਅਚਲ newp
                = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, ਵਿਧੀ, rhsp};
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ adtypep = VN_CAST(ddtypep, ਕਤਾਰ_DType)) {
            // SELBIT(array, index) -> CMETHODCALL(queue, "at", index)
            AstCMethodHard* newp;
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ backnessp = sel_ਕਤਾਰ_Backness(rhsp)) {
                ਅਚਲ VCMethod ਵਿਧੀ = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND_ਪਿੱਠ
                                            : VCMethod::ARRAY_AT_ਪਿੱਠ;
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, ਵਿਧੀ, backnessp};
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ VCMethod ਵਿਧੀ = nodep->access().isWriteOrRW()
                                            ? VCMethod::DYN_AT_WRITE_APPEND
                                            : VCMethod::ARRAY_AT;
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, ਵਿਧੀ, rhsp};
            }
            newp->dtypeFrom(adtypep->subDTypep());  // Need to strip off queue reference
            UINFOTREE(9, newp, "", "SELBTq");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, BasicDType) && ddtypep->isString()) {
            // SELBIT(string, index) -> GETC(string, index)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = fromp;
            ਜਦੋਂ (ਸੱਚ) {
                ਜੇ (AstMemberSel* ਅਚਲ memberselp = VN_CAST(exprp, MemberSel)) {
                    exprp = memberselp->fromp();
                } ਨਹੀਂਤਾਂ ਜੇ (AstStructSel* ਅਚਲ structselp = VN_CAST(exprp, StructSel)) {
                    exprp = structselp->fromp();
                } ਨਹੀਂਤਾਂ {
                    ਰੋਕੋ;
                }
            }
            ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(exprp, ਗੰਢ_Var_Ref);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            ਜੇ (!varrefp || varrefp->access().isReadOnly()) {
                newp = ਨਵਾਂ AstGetcN{nodep->fileline(), fromp, rhsp};
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ AstGetcRefN{nodep->fileline(), fromp, rhsp};
            }
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, BasicDType)) {
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ਅਚਲ newp
                = ਨਵਾਂ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ਦਾ(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, ਗੰਢ_UOr_Struct_DType)) {  // A bit from the packed struct
            // SELBIT(range, index) -> SEL(array, index, 1)
            AstSel* ਅਚਲ newp
                = ਨਵਾਂ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ਦਾ(rhsp, fromRange), 1};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELBTn");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal bit or array select; type already selected, or bad dimension: "
                           << "data type is" << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
        ਜੇ (!rhsp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sel_ਕੱਢੋ* nodep) override {
        // Select of a range specified part of an array, i.e. "array[2:3]"
        // SELEXTRACT(from,msb,lsb) -> SEL(from, lsb, 1+msb-lsb)
        // This select style has a (msb or lsb) and width
        UINFO(6, "SELEXTRACT " << nodep);
        // UINFOTREE(9, nodep, "", "SELEX0");
        // Below 2 lines may change nodep->widthp()
        V3Const::constifyParamsNoWarnEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsNoWarnEdit(nodep->rightp());  // May relink pointed to node
        // UINFOTREE(9, nodep, "", "SELEX3");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ From_ਅੰਕੜੇ fromdata = from_ਅੰਕੜੇ_For_Array(nodep, fromp);
        Ast_ਗੰਢ_DType* ਅਚਲ ddtypep = fromdata.m_dtypep;
        ਅਚਲ VNumRange fromRange = fromdata.m_fromRange;
        ਜੇ (VN_IS(ddtypep, ਕਤਾਰ_DType)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ qleftp = nodep->leftp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ qrightp = nodep->rightp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ qleftBacknessp = sel_ਕਤਾਰ_Backness(qleftp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ qrightBacknessp = sel_ਕਤਾਰ_Backness(qrightp);
            // Use special methods to refer to back rather than math using
            // queue size, this allows a single queue reference, to support
            // for equations in side effects that select the queue to
            // operate upon.
            ਅਚਲ VCMethod ਵਿਧੀ = (qleftBacknessp    ? VCMethod::DYN_ਟੁਕੜਾ_ਪਿੱਠ_ਪਿੱਠ
                                     : qrightBacknessp ? VCMethod::DYN_ਟੁਕੜਾ_FRONT_ਪਿੱਠ
                                                       : VCMethod::DYN_ਟੁਕੜਾ);
            AstCMethodHard* ਅਚਲ newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, ਵਿਧੀ};
            ਜੇ (qleftBacknessp) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(qleftp), qleftp);
                newp->ਜੋੜੋ_Pinsp(qleftBacknessp);
            } ਨਹੀਂਤਾਂ {
                newp->ਜੋੜੋ_Pinsp(qleftp);
            }
            ਜੇ (qrightBacknessp) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(qrightp), qrightp);
                newp->ਜੋੜੋ_Pinsp(qrightBacknessp);
            } ਨਹੀਂਤਾਂ {
                newp->ਜੋੜੋ_Pinsp(qrightp);
            }
            newp->dtypep(ddtypep);
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            newp->protect(ਝੂਠ);
            UINFO(6, "   new " << newp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        // Non-queue
        checkConstantOrReplace(nodep->leftp(),
                               "First value of [a:b] isn't a constant, maybe you want +: or -:");
        checkConstantOrReplace(nodep->rightp(),
                               "Second value of [a:b] isn't a constant, maybe you want +: or -:");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ msbp = nodep->leftp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = nodep->rightp()->unlink_Fr_ਪਿੱਠ();
        int32_t msb = VN_AS(msbp, Const)->ਮੰਜ਼ਿਲ_SInt();
        int32_t lsb = VN_AS(lsbp, Const)->ਮੰਜ਼ਿਲ_SInt();
        ਅਚਲ int32_t elem = (msb > lsb) ? (msb - lsb + 1) : (lsb - msb + 1);
        ਜੇ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice extraction
            ਜੇ (fromRange.elements() == elem
                && fromRange.lo() == lsb) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ਅਚਲ newp = ਨਵਾਂ AstArraySel{nodep->fileline(), fromp, lsbp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ {  //ਟੁਕੜਾ
                Ast_ਟੁਕੜਾ_Sel* ਅਚਲ newp
                    = ਨਵਾਂ Ast_ਟੁਕੜਾ_Sel{nodep->fileline(), fromp,
                                      VNumRange{msb - fromRange.lo(), lsb - fromRange.lo()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstPackArrayDType* ਅਚਲ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
            // SELEXTRACT(array, msb, lsb) -> SEL(array,
            //             lsb*width-of-subindex, width-of-subindex*(msb-lsb))
            UASSERT_OBJ(!(!fromRange.elements() || (adtypep->ਚੌੜਾਈ() % fromRange.elements()) != 0),
                        adtypep,
                        "Array extraction with width miscomputed " << adtypep->ਚੌੜਾਈ() << "/"
                                                                   << fromRange.elements());
            // Below code assumes descending bit range; just works out if we swap
            ਜੇ (fromRange.ascending()) std::ਅਦਲਾ_ਬਦਲੀ(msb, lsb);
            ਜੇ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::ਅਦਲਾ_ਬਦਲੀ(msb, lsb);
            }
            ਅਚਲ ਪੂਰਨ elwidth = adtypep->ਚੌੜਾਈ() / fromRange.elements();
            AstSel* ਅਚਲ newp
                = ਨਵਾਂ AstSel{nodep->fileline(), fromp,
                             newMulConst(nodep->fileline(), elwidth, new_Sub_Lsb_ਦਾ(lsbp, fromRange)),
                             (msb - lsb + 1) * elwidth};
            newp->declRange(fromRange);
            newp->decl_El_ਚੌੜਾਈ(elwidth);
            newp->dtypeFrom(ਟੁਕੜਾ_DType(adtypep, msb, lsb));
            // UINFOTREE(9, newp, "", "EXTBTn");
            UASSERT_OBJ(newp->ਚੌੜਾਈ_Min() == newp->ਚੌੜਾਈ_Const(), nodep, "Width mismatch");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, BasicDType)) {
            // Below code assumes descending bit range; just works out if we swap
            ਜੇ (fromRange.ascending()) std::ਅਦਲਾ_ਬਦਲੀ(msb, lsb);
            ਜੇ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::ਅਦਲਾ_ਬਦਲੀ(msb, lsb);
            }
            AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ਦਾ(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, ਗੰਢ_UOr_Struct_DType)) {
            // Classes don't have an ascending range
            ਜੇ (lsb > msb) {
                nodep->v3warn(
                    SELRANGE,
                    "[" << msb << ":" << lsb
                        << "] Slice range has ascending bit ordering, perhaps you wanted [" << lsb
                        << ":" << msb << "]");
                std::ਅਦਲਾ_ਬਦਲੀ(msb, lsb);
            }
            AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{nodep->fileline(), fromp, new_Sub_Lsb_ਦਾ(lsbp, fromRange),
                                            msb + 1 - lsb};
            newp->declRange(fromRange);
            UINFO(6, "   new " << newp);
            // UINFOTREE(9, newp, "", "SELEXnew");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal range select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            UINFO(1, "    Related ddtype: " << ddtypep);
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ਜੇ (!fromp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(fromp), fromp);
        ਜੇ (!msbp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(msbp), msbp);
        ਜੇ (!lsbp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(lsbp), lsbp);
    }

    ਖਾਲੀ replaceSelPlusMinus(Ast_ਗੰਢ_Pre_Sel* nodep) {
        // Select of a range specified with +: or -:, i.e. "array[2+:3], [2-:3]"
        // This select style has a lsb and width
        UINFO(6, "SELPLUS/MINUS " << nodep);
        // Below 2 lines may change nodep->widthp()
        UINFOTREE(9, nodep, "", "SELPM0");
        V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(nodep->rhsp());  // constifyEdit doesn't ensure widths finished
        V3Const::constifyEdit(nodep->rhsp());  // May relink pointed to node, ok if not const
        V3Const::constifyParamsEdit(nodep->thsp());  // May relink pointed to node
        checkConstantOrReplace(nodep->thsp(),
                               "Width of :+ or :- bit slice range isn't a constant");
        UINFOTREE(9, nodep, "", "SELPM3");
        // Now replace it with an AstSel
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ widthp = nodep->thsp()->unlink_Fr_ਪਿੱਠ();
        warnTri(rhsp);
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = VN_AS(widthp, Const)->ਮੰਜ਼ਿਲ_SInt();
        ਜੇ (ਚੌੜਾਈ > (1 << 28)) {
            nodep->v3error("Width of :+ or :- is huge; vector of over 1 billion bits: "
                           << widthp->prettyName());
        }
        ਜੇ (ਚੌੜਾਈ < 0) nodep->v3error("Width of :+ or :- is < 0: " << widthp->prettyName());
        ਅਚਲ From_ਅੰਕੜੇ fromdata = from_ਅੰਕੜੇ_For_Array(nodep, fromp);
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ddtypep = fromdata.m_dtypep;
        ਅਚਲ VNumRange fromRange = fromdata.m_fromRange;
        ਜੇ (VN_IS(ddtypep, UnpackArrayDType)) {
            // Slice +: and -: extraction
            ਜੇ (fromRange.elements() == ਚੌੜਾਈ && VN_IS(rhsp, Const)
                && VN_AS(rhsp, Const)->ਮੰਜ਼ਿਲ_SInt()
                       == fromRange.lo()) {  // Extracting whole of original array
                nodep->replaceWith(fromp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (fromRange.elements() == 1) {  // Extracting single element
                AstArraySel* ਅਚਲ newp = ਨਵਾਂ AstArraySel{nodep->fileline(), fromp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(rhsp, Const)) {  //ਟੁਕੜਾ
                ਅਚਲ int32_t rhs = VN_AS(rhsp, Const)->ਮੰਜ਼ਿਲ_SInt();
                // down array: lsb/lo +: width
                // down array: msb/hi -: width
                // up array:   msb/lo +: width
                // up array:   lsb/hi -: width
                ਅਚਲ int32_t msb
                    = (VN_IS(nodep, SelPlus) ? rhs + ਚੌੜਾਈ - 1 : rhs) - fromRange.lo();
                ਅਚਲ int32_t lsb
                    = (VN_IS(nodep, SelPlus) ? rhs : rhs - ਚੌੜਾਈ + 1) - fromRange.lo();
                Ast_ਟੁਕੜਾ_Sel* ਅਚਲ newp = ਨਵਾਂ Ast_ਟੁਕੜਾ_Sel{
                    nodep->fileline(), fromp, VNumRange{msb, lsb, fromRange.ascending()}};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Slice of non-constant bounds");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ddtypep, BasicDType) || VN_IS(ddtypep, PackArrayDType)
                   || (VN_IS(ddtypep, ਗੰਢ_UOr_Struct_DType)
                       && VN_AS(ddtypep, ਗੰਢ_UOr_Struct_DType)->packedUnsup())) {
            ਪੂਰਨ elwidth = 1;
            ਪੂਰਨ newwidth = ਚੌੜਾਈ;
            ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ adtypep = VN_CAST(ddtypep, PackArrayDType)) {
                elwidth = adtypep->ਚੌੜਾਈ() / fromRange.elements();
                newwidth = ਚੌੜਾਈ * elwidth;
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newlsbp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (VN_IS(nodep, SelPlus)) {
                ਜੇ (fromRange.ascending()) {
                    // SELPLUS(from,lsb,width) -> SEL(from, (vector_msb-width+1)-sel, width)
                    newlsbp = newSubNeg((fromRange.hi() - ਚੌੜਾਈ + 1), rhsp);
                } ਨਹੀਂਤਾਂ {
                    // SELPLUS(from,lsb,width) -> SEL(from, lsb-vector_lsb, width)
                    newlsbp = newSubNeg(rhsp, fromRange.lo());
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, SelMinus)) {
                ਜੇ (fromRange.ascending()) {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-[bit])
                    newlsbp = newSubNeg(fromRange.hi(), rhsp);
                } ਨਹੀਂਤਾਂ {
                    // SELMINUS(from,msb,width) -> SEL(from, msb-(width-1)-lsb#)
                    newlsbp = newSubNeg(rhsp, fromRange.lo() + (ਚੌੜਾਈ - 1));
                }
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Bad Case");
            }
            ਜੇ (elwidth != 1) newlsbp = newMulConst(nodep->fileline(), elwidth, newlsbp);
            AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{nodep->fileline(), fromp, newlsbp, newwidth};
            newp->declRange(fromRange);
            newp->decl_El_ਚੌੜਾਈ(elwidth);
            UINFO(6, "   new " << newp);
            UINFOTREE(9, newp, "", "SELNEW");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {  // nullptr=bad extract, or unknown node type
            nodep->v3error("Illegal +: or -: select; type already selected, or bad dimension: "
                           << "data type is " << fromdata.m_errp->prettyDTypeNameQ());
            // How to recover?  We'll strip a dimension.
            nodep->replaceWith(fromp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
        // delete whatever we didn't use in reconstruction
        ਜੇ (!fromp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(fromp), fromp);
        ਜੇ (!rhsp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
        ਜੇ (!widthp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(widthp), widthp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSelPlus* nodep) override { replaceSelPlusMinus(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSelMinus* nodep) override { replaceSelPlusMinus(nodep); }
    // If adding new visitors, ensure V3Width's visit(TYPE) calls into here

    //--------------------
    // Default
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {  // LCOV_EXCL_LINE
        // See notes above; we never iterate
        nodep->v3fatalSrc("Shouldn't iterate in V3WidthSel");
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਚੌੜਾਈ_Sel_Visitor() = ਮੂਲ;
    ~ਚੌੜਾਈ_Sel_Visitor() override = ਮੂਲ;
    Ast_ਗੰਢ* mainAcceptEdit(Ast_ਗੰਢ* nodep) { ਵਾਪਸ iterateSubtreeReturnEdits(nodep); }
};

//######################################################################
// Width class functions

Ast_ਗੰਢ* V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Sel_No_Iter_Edit(Ast_ਗੰਢ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    ਚੌੜਾਈ_Sel_Visitor visitor;
    nodep = visitor.mainAcceptEdit(nodep);
    ਵਾਪਸ nodep;
}

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ iterateChildren
