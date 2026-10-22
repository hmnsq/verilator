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
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE

    // METHODS

    فراغ وجهو_Cast(Ast_ڳنڍ_اظهار* nodep, صحيح needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_پٺ(&relinkHandle);
        //
        AstCCast* ثابت castp
            = جديد AstCCast{nodep->fileline(), nodep, needsize, nodep->ويڪر_Min()};
        UINFO(4, "  MadeCast " << static_cast<فراغ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ساكن صحيح cast_سائيز(صحيح ويڪر) {
        إذا (ويڪر > VL_IDATASIZE && ويڪر <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا إذا (ويڪر <= 8) {
            أرجع 8;
        } وإلا إذا (ويڪر <= 16) {
            أرجع 16;
        } وإلا {
            أرجع VL_IDATASIZE;
        }
    }
    ساكن صحيح cast_سائيز(ثابت Ast_ڳنڍ* nodep) { أرجع cast_سائيز(nodep->ويڪر()); }
    فراغ ensureCast(Ast_ڳنڍ_اظهار* nodep) {
        إذا (cast_سائيز(nodep->backp()) != cast_سائيز(nodep) || !nodep->user1()) {
            إذا (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                وجهو_Cast(nodep, cast_سائيز(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    فراغ ensure_Cast_قدر(Ast_ڳنڍ_اظهار* valuep, ثابت Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (dtypep->isIntegralOrPacked() && dtypep->ويڪر() <= VL_QUADSIZE) {
            وجهو_Cast(valuep, cast_سائيز(dtypep->ويڪر()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    فراغ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        إذا (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            وجهو_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    فراغ ensureNullChecked(Ast_ڳنڍ_اظهار* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        إذا (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_پٺ(&relinkHandle);
            relinkHandle.relink(جديد AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->سائيز_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        إذا (nodep->سائيز_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->سائيز_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    فراغ گهمو(AstCond* nodep) override {
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
            ثابت AstClass* ثابت common_بنياد_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            إذا (thenClassDtypep->classp() != common_بنياد_Classp) {
                Ast_ڳنڍ_اظهار* thenp = nodep->thenp()->unlink_Fr_پٺ();
                nodep->thenp(جديد AstCCast{thenp->fileline(), thenp, nodep});
            }
            إذا (elseClassDtypep->classp() != common_بنياد_Classp) {
                Ast_ڳنڍ_اظهار* elsep = nodep->elsep()->unlink_Fr_پٺ();
                nodep->elsep(جديد AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        گهمو(static_cast<Ast_ڳنڍ_Triop*>(nodep));
    }
    فراغ گهمو(Ast_ڳنڍ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        إذا (nodep->سائيز_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->سائيز_Matters_Rhs()) ensureCast(nodep->rhsp());
        إذا (nodep->سائيز_Matters_Ths()) ensureCast(nodep->thsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        إذا (nodep->سائيز_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->سائيز_Matters_Rhs()) ensureCast(nodep->rhsp());
        إذا (nodep->سائيز_Matters_Ths()) ensureCast(nodep->thsp());
        إذا (nodep->سائيز_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    فراغ گهمو(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    فراغ گهمو(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ گهمو(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ گهمو(Ast_Cons_Struct_شي* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_قدر(nodep->valuep(), nodep->memberp());
    }
    فراغ گهمو(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    فراغ گهمو(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->lhsp()->ويڪر_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            وجهو_Cast(nodep->lhsp(), cast_سائيز(nodep));
        } وإلا {
            إذا (nodep->سائيز_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    فراغ گهمو(AstVarRef* nodep) override {
        ثابت Ast_ڳنڍ* ثابت backp = nodep->backp();
        إذا (nodep->access().isReadOnly() && VN_IS(backp, ڳنڍ_اظهار) && !VN_IS(backp, CCast)
            && !VN_IS(backp, ڳنڍ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->ويڪر() && cast_سائيز(nodep) != cast_سائيز(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            وجهو_Cast(nodep, cast_سائيز(nodep));
        }
        nodep->user1(1);
    }
    فراغ گهمو(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    فراغ گهمو(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    فراغ گهمو(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(سچ);
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(سچ);
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(سچ);
    }
    فراغ گهمو(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(سچ);
    }

    // NOPs
    فراغ گهمو(AstVar*) override {}

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_وڻ("cast", 0, dump_وڻ_Either_Level() >= 3);
}
