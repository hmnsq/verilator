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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

جماعت CastVisitor final : عوامی VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE

    // METHODS

    خالی داخل_کرو_Cast(Ast_گرہ_اظہاریہ* nodep, صحیح needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_عقب(&relinkHandle);
        //
        AstCCast* ثابت castp
            = نیا AstCCast{nodep->fileline(), nodep, needsize, nodep->چوڑائی_Min()};
        UINFO(4, "  MadeCast " << static_cast<خالی*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ساکن صحیح cast_سائز(صحیح چوڑائی) {
        اگر (چوڑائی > VL_IDATASIZE && چوڑائی <= VL_QUADSIZE) {
            واپس VL_QUADSIZE;
        } ورنہ اگر (چوڑائی <= 8) {
            واپس 8;
        } ورنہ اگر (چوڑائی <= 16) {
            واپس 16;
        } ورنہ {
            واپس VL_IDATASIZE;
        }
    }
    ساکن صحیح cast_سائز(ثابت Ast_گرہ* nodep) { واپس cast_سائز(nodep->چوڑائی()); }
    خالی ensureCast(Ast_گرہ_اظہاریہ* nodep) {
        اگر (cast_سائز(nodep->backp()) != cast_سائز(nodep) || !nodep->user1()) {
            اگر (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                داخل_کرو_Cast(nodep, cast_سائز(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    خالی ensure_Cast_قدر(Ast_گرہ_اظہاریہ* valuep, ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (dtypep->isIntegralOrPacked() && dtypep->چوڑائی() <= VL_QUADSIZE) {
            داخل_کرو_Cast(valuep, cast_سائز(dtypep->چوڑائی()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    خالی ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        اگر (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            داخل_کرو_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    خالی ensureNullChecked(Ast_گرہ_اظہاریہ* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        اگر (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_عقب(&relinkHandle);
            relinkHandle.relink(نیا AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        اگر (nodep->سائز_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    خالی دورہ(Ast_گرہ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        اگر (nodep->سائز_Matters_Lhs()) ensureCast(nodep->lhsp());
        اگر (nodep->سائز_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    خالی دورہ(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ثابت AstClassRefDType* ثابت thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ثابت AstClassRefDType* ثابت elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ثابت منطقی castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        اگر (castRequired) {
            ثابت AstClass* ثابت common_بنیاد_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            اگر (thenClassDtypep->classp() != common_بنیاد_Classp) {
                Ast_گرہ_اظہاریہ* thenp = nodep->thenp()->unlink_Fr_عقب();
                nodep->thenp(نیا AstCCast{thenp->fileline(), thenp, nodep});
            }
            اگر (elseClassDtypep->classp() != common_بنیاد_Classp) {
                Ast_گرہ_اظہاریہ* elsep = nodep->elsep()->unlink_Fr_عقب();
                nodep->elsep(نیا AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        دورہ(static_cast<Ast_گرہ_Triop*>(nodep));
    }
    خالی دورہ(Ast_گرہ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        اگر (nodep->سائز_Matters_Lhs()) ensureCast(nodep->lhsp());
        اگر (nodep->سائز_Matters_Rhs()) ensureCast(nodep->rhsp());
        اگر (nodep->سائز_Matters_Ths()) ensureCast(nodep->thsp());
    }
    خالی دورہ(Ast_گرہ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        اگر (nodep->سائز_Matters_Lhs()) ensureCast(nodep->lhsp());
        اگر (nodep->سائز_Matters_Rhs()) ensureCast(nodep->rhsp());
        اگر (nodep->سائز_Matters_Ths()) ensureCast(nodep->thsp());
        اگر (nodep->سائز_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    خالی دورہ(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    خالی دورہ(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    خالی دورہ(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    خالی دورہ(Ast_Cons_Struct_شے* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_قدر(nodep->valuep(), nodep->memberp());
    }
    خالی دورہ(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) {
            ensure_Cast_قدر(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    خالی دورہ(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        اگر (nodep->lhsp()->چوڑائی_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            داخل_کرو_Cast(nodep->lhsp(), cast_سائز(nodep));
        } ورنہ {
            اگر (nodep->سائز_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    خالی دورہ(AstVarRef* nodep) override {
        ثابت Ast_گرہ* ثابت backp = nodep->backp();
        اگر (nodep->access().isReadOnly() && VN_IS(backp, گرہ_اظہاریہ) && !VN_IS(backp, CCast)
            && !VN_IS(backp, گرہ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->چوڑائی() && cast_سائز(nodep) != cast_سائز(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            داخل_کرو_Cast(nodep, cast_سائز(nodep));
        }
        nodep->user1(1);
    }
    خالی دورہ(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    خالی دورہ(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    خالی دورہ(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(سچ);
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(سچ);
    }
    خالی دورہ(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(سچ);
    }
    خالی دورہ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(سچ);
    }

    // NOPs
    خالی دورہ(AstVar*) override {}

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = طےشدہ;
};

//######################################################################
// Cast class functions

خالی V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("cast", 0, dump_درخت_Either_Level() >= 3);
}
