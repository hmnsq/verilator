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
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE

    // METHODS

    فراغ ورننباسه_Cast(Ast_غوټه_بیان* nodep, صحيح needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_شا(&relinkHandle);
        //
        AstCCast* ثابت castp
            = جديد AstCCast{nodep->fileline(), nodep, needsize, nodep->پلنوالی_Min()};
        UINFO(4, "  MadeCast " << static_cast<فراغ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ساكن صحيح cast_اندازه(صحيح پلنوالی) {
        إذا (پلنوالی > VL_IDATASIZE && پلنوالی <= VL_QUADSIZE) {
            أرجع VL_QUADSIZE;
        } وإلا إذا (پلنوالی <= 8) {
            أرجع 8;
        } وإلا إذا (پلنوالی <= 16) {
            أرجع 16;
        } وإلا {
            أرجع VL_IDATASIZE;
        }
    }
    ساكن صحيح cast_اندازه(ثابت Ast_غوټه* nodep) { أرجع cast_اندازه(nodep->پلنوالی()); }
    فراغ ensureCast(Ast_غوټه_بیان* nodep) {
        إذا (cast_اندازه(nodep->backp()) != cast_اندازه(nodep) || !nodep->user1()) {
            إذا (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ورننباسه_Cast(nodep, cast_اندازه(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    فراغ ensure_Cast_ارزښت(Ast_غوټه_بیان* valuep, ثابت Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (dtypep->isIntegralOrPacked() && dtypep->پلنوالی() <= VL_QUADSIZE) {
            ورننباسه_Cast(valuep, cast_اندازه(dtypep->پلنوالی()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    فراغ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        إذا (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ورننباسه_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    فراغ ensureNullChecked(Ast_غوټه_بیان* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        إذا (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_شا(&relinkHandle);
            relinkHandle.relink(جديد AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->اندازه_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        إذا (nodep->اندازه_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->اندازه_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    فراغ لیدنه(AstCond* nodep) override {
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
            ثابت AstClass* ثابت common_بنسټ_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            إذا (thenClassDtypep->classp() != common_بنسټ_Classp) {
                Ast_غوټه_بیان* thenp = nodep->thenp()->unlink_Fr_شا();
                nodep->thenp(جديد AstCCast{thenp->fileline(), thenp, nodep});
            }
            إذا (elseClassDtypep->classp() != common_بنسټ_Classp) {
                Ast_غوټه_بیان* elsep = nodep->elsep()->unlink_Fr_شا();
                nodep->elsep(جديد AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        لیدنه(static_cast<Ast_غوټه_Triop*>(nodep));
    }
    فراغ لیدنه(Ast_غوټه_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        إذا (nodep->اندازه_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->اندازه_Matters_Rhs()) ensureCast(nodep->rhsp());
        إذا (nodep->اندازه_Matters_Ths()) ensureCast(nodep->thsp());
    }
    فراغ لیدنه(Ast_غوټه_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        إذا (nodep->اندازه_Matters_Lhs()) ensureCast(nodep->lhsp());
        إذا (nodep->اندازه_Matters_Rhs()) ensureCast(nodep->rhsp());
        إذا (nodep->اندازه_Matters_Ths()) ensureCast(nodep->thsp());
        إذا (nodep->اندازه_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    فراغ لیدنه(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    فراغ لیدنه(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_ارزښت(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ لیدنه(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_ارزښت(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ لیدنه(Ast_Cons_Struct_توکی* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_ارزښت(nodep->valuep(), nodep->memberp());
    }
    فراغ لیدنه(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_ارزښت(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    فراغ لیدنه(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->lhsp()->پلنوالی_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ورننباسه_Cast(nodep->lhsp(), cast_اندازه(nodep));
        } وإلا {
            إذا (nodep->اندازه_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        ثابت Ast_غوټه* ثابت backp = nodep->backp();
        إذا (nodep->access().isReadOnly() && VN_IS(backp, غوټه_بیان) && !VN_IS(backp, CCast)
            && !VN_IS(backp, غوټه_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->پلنوالی() && cast_اندازه(nodep) != cast_اندازه(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ورننباسه_Cast(nodep, cast_اندازه(nodep));
        }
        nodep->user1(1);
    }
    فراغ لیدنه(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    فراغ لیدنه(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    فراغ لیدنه(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ریښتیا);
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ریښتیا);
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ریښتیا);
    }
    فراغ لیدنه(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ریښتیا);
    }

    // NOPs
    فراغ لیدنه(AstVar*) override {}

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = افتراضي;
};

//######################################################################
// Cast class functions

فراغ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("cast", 0, dump_ونه_Either_Level() >= 3);
}
