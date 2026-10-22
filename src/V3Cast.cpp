// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add C++ casts across expression size changes
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Cast's Transformations:
//
// Each module:
//      For each expression, if above expression requires 32 bits,
//      and this isn't, cast to 32 bits.
//      Likewise for 64 bit expressions.
//
// C++ rules:
//      Integral promotions allow conversion to larger int.  Unsigned is only
//      used if a int would not fit the value.
//
//      Bools converts to int, not unsigned.
//
//      Most operations return unsigned if either operand is unsigned.
//
//      Unsignedness can be lost on results of the below operations, as they
//      may need the sign bit for proper operation:
//              /, %, /=, %=, <, <=, >, or >=
//
//      Signed values are always sign extended on promotion or right shift,
//      even if assigning to a unsigned.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

صنف CastVisitor final : عام VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE

    // METHODS

    فراغ داخل_کرو_Cast(Ast_گرہ_اظہاریہ* nodep, صحيح needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_پٹھ(&relinkHandle);
        //
        AstCCast* ثابت castp
            = جديد AstCCast{nodep->fileline(), nodep, needsize, nodep->چوڑائی_Min()};
        UINFO(4, "  MadeCast " << static_cast<فراغ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ساكن صحيح cast_ناپ(صحيح چوڑائی) {
        إذا (چوڑائی > VL_IDATASIZE && چوڑائی <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا إذا (چوڑائی <= 8) {
            أرجع 8;
        } وإلا إذا (چوڑائی <= 16) {
            أرجع 16;
        } وإلا {
            أرجع VL_IDATASIZE;
        }
    }
    ساكن صحيح cast_ناپ(ثابت Ast_گرہ* nodep) { أرجع cast_ناپ(nodep->چوڑائی()); }
    فراغ ensureCast(Ast_گرہ_اظہاریہ* nodep) {
        إذا (cast_ناپ(nodep->backp()) != cast_ناپ(nodep) || !nodep->user1()) {
            إذا (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                داخل_کرو_Cast(nodep, cast_ناپ(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    فراغ ensure_Cast_قدر(Ast_گرہ_اظہاریہ* valuep, ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (dtypep->isIntegralOrPacked() && dtypep->چوڑائی() <= VL_QUADSIZE) {
            داخل_کرو_Cast(valuep, cast_ناپ(dtypep->چوڑائی()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    فراغ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        إذا (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            داخل_کرو_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    فراغ ensureNullChecked(Ast_گرہ_اظہاریہ* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        إذا (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_پٹھ(&relinkHandle);
            relinkHandle.relink(جديد AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->ناپ_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    فراغ ویکھو(Ast_گرہ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        إذا (nodep->ناپ_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->ناپ_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    فراغ ویکھو(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ثابت AstClassRefDType* ثابت thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ثابت AstClassRefDType* ثابت elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ثابت منطقي castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        إذا (castRequired) {
            ثابت AstClass* ثابت common_ادھار_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            إذا (thenClassDtypep->classp() != common_ادھار_Classp) {
                Ast_گرہ_اظہاریہ* thenp = nodep->thenp()->unlink_Fr_پٹھ();
                nodep->thenp(جديد AstCCast{thenp->fileline(), thenp, nodep});
            }
            إذا (elseClassDtypep->classp() != common_ادھار_Classp) {
                Ast_گرہ_اظہاریہ* elsep = nodep->elsep()->unlink_Fr_پٹھ();
                nodep->elsep(جديد AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        ویکھو(static_cast<Ast_گرہ_Triop*>(nodep));
    }
    فراغ ویکھو(Ast_گرہ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        إذا (nodep->ناپ_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->ناپ_Matters_Rhs()) ensureCast(nodep->rhsp());
        إذا (nodep->ناپ_Matters_Ths()) ensureCast(nodep->thsp());
    }
    فراغ ویکھو(Ast_گرہ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        إذا (nodep->ناپ_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->ناپ_Matters_Rhs()) ensureCast(nodep->rhsp());
        إذا (nodep->ناپ_Matters_Ths()) ensureCast(nodep->thsp());
        إذا (nodep->ناپ_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    فراغ ویکھو(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    فراغ ویکھو(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ ویکھو(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ ویکھو(Ast_Cons_Struct_شے* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_قدر(nodep->valuep(), nodep->memberp());
    }
    فراغ ویکھو(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ ویکھو(Ast_اظہاریہ_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    فراغ ویکھو(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->lhsp()->چوڑائی_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            داخل_کرو_Cast(nodep->lhsp(), cast_ناپ(nodep));
        } وإلا {
            إذا (nodep->ناپ_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        إذا (nodep->access().isReadOnly() && VN_IS(backp, گرہ_اظہاریہ) && !VN_IS(backp, CCast)
            && !VN_IS(backp, گرہ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->چوڑائی() && cast_ناپ(nodep) != cast_ناپ(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            داخل_کرو_Cast(nodep, cast_ناپ(nodep));
        }
        nodep->user1(1);
    }
    فراغ ویکھو(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    فراغ ویکھو(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    فراغ ویکھو(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(سچ);
    }
    فراغ ویکھو(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(سچ);
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(سچ);
    }
    فراغ ویکھو(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(سچ);
    }

    // NOPs
    فراغ ویکھو(AstVar*) override {}

    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = افتراضي;
};

//######################################################################
// Cast class functions

فراغ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("cast", 0, dump_درخت_Either_Level() >= 3);
}
