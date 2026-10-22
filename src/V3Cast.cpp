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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

শ্রেণী CastVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ধ্রুবক VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    শূন্য ঢোকাও_Cast(Ast_গ্রন্থি_রাশিমালা* nodep, পূর্ণসংখ্যা needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* ধ্রুবক castp
            = নতুন AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<শূন্য*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    স্থির পূর্ণসংখ্যা cast_আকার(পূর্ণসংখ্যা width) {
        যদি (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            ফেরত VL_QUADSIZE;
        } অন্যথা যদি (width <= 8) {
            ফেরত 8;
        } অন্যথা যদি (width <= 16) {
            ফেরত 16;
        } অন্যথা {
            ফেরত VL_IDATASIZE;
        }
    }
    স্থির পূর্ণসংখ্যা cast_আকার(ধ্রুবক Ast_গ্রন্থি* nodep) { ফেরত cast_আকার(nodep->width()); }
    শূন্য ensureCast(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (cast_আকার(nodep->backp()) != cast_আকার(nodep) || !nodep->user1()) {
            যদি (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ঢোকাও_Cast(nodep, cast_আকার(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    শূন্য ensure_Cast_মান(Ast_গ্রন্থি_রাশিমালা* valuep, ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            ঢোকাও_Cast(valuep, cast_আকার(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    শূন্য ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        যদি (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ঢোকাও_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    শূন্য ensureNullChecked(Ast_গ্রন্থি_রাশিমালা* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        যদি (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(নতুন AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        যদি (nodep->আকার_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        যদি (nodep->আকার_Matters_Lhs()) ensureCast(nodep->lhsp());
        যদি (nodep->আকার_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    শূন্য visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ধ্রুবক AstClassRefDType* ধ্রুবক thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ধ্রুবক AstClassRefDType* ধ্রুবক elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ধ্রুবক যৌক্তিক castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        যদি (castRequired) {
            ধ্রুবক AstClass* ধ্রুবক commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            যদি (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_গ্রন্থি_রাশিমালা* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(নতুন AstCCast{thenp->fileline(), thenp, nodep});
            }
            যদি (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_গ্রন্থি_রাশিমালা* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(নতুন AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_গ্রন্থি_Triop*>(nodep));
    }
    শূন্য visit(Ast_গ্রন্থি_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        যদি (nodep->আকার_Matters_Lhs()) ensureCast(nodep->lhsp());
        যদি (nodep->আকার_Matters_Rhs()) ensureCast(nodep->rhsp());
        যদি (nodep->আকার_Matters_Ths()) ensureCast(nodep->thsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        যদি (nodep->আকার_Matters_Lhs()) ensureCast(nodep->lhsp());
        যদি (nodep->আকার_Matters_Rhs()) ensureCast(nodep->rhsp());
        যদি (nodep->আকার_Matters_Ths()) ensureCast(nodep->thsp());
        যদি (nodep->আকার_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    শূন্য visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    শূন্য visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->defaultp()) {
            ensure_Cast_মান(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    শূন্য visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->defaultp()) {
            ensure_Cast_মান(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    শূন্য visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_মান(nodep->valuep(), nodep->memberp());
    }
    শূন্য visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        যদি (nodep->defaultp()) {
            ensure_Cast_মান(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    শূন্য visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        যদি (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ঢোকাও_Cast(nodep->lhsp(), cast_আকার(nodep));
        } অন্যথা {
            যদি (nodep->আকার_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    শূন্য visit(AstVarRef* nodep) override {
        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক backp = nodep->backp();
        যদি (nodep->access().isReadOnly() && VN_IS(backp, গ্রন্থি_রাশিমালা) && !VN_IS(backp, CCast)
            && !VN_IS(backp, গ্রন্থি_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && cast_আকার(nodep) != cast_আকার(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ঢোকাও_Cast(nodep, cast_আকার(nodep));
        }
        nodep->user1(1);
    }
    শূন্য visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    শূন্য visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    শূন্য visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(সত্য);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(সত্য);
    }
    শূন্য visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(সত্য);
    }
    শূন্য visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(সত্য);
    }

    // NOPs
    শূন্য visit(AstVar*) override {}

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Cast class functions

শূন্য V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("cast", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
