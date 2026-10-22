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
    ثابت VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    فراغ أدرج_Cast(Ast_عقدة_تعبير* nodep, صحيح needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* ثابت castp
            = جديد AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<فراغ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ساكن صحيح castSize(صحيح width) {
        إذا (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            إرجاع VL_QUADSIZE;
        } وإلا إذا (width <= 8) {
            إرجاع 8;
        } وإلا إذا (width <= 16) {
            إرجاع 16;
        } وإلا {
            إرجاع VL_IDATASIZE;
        }
    }
    ساكن صحيح castSize(ثابت Ast_عقدة* nodep) { إرجاع castSize(nodep->width()); }
    فراغ ensureCast(Ast_عقدة_تعبير* nodep) {
        إذا (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            إذا (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                أدرج_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    فراغ ensure_Cast_قيمة(Ast_عقدة_تعبير* valuep, ثابت Ast_عقدة_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            أدرج_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    فراغ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        إذا (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            أدرج_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    فراغ ensureNullChecked(Ast_عقدة_تعبير* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        إذا (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(جديد AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    فراغ visit(Ast_عقدة_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        إذا (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        إذا (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    فراغ visit(AstCond* nodep) override {
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
            ثابت AstClass* ثابت commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            إذا (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_عقدة_تعبير* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(جديد AstCCast{thenp->fileline(), thenp, nodep});
            }
            إذا (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_عقدة_تعبير* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(جديد AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_عقدة_Triop*>(nodep));
    }
    فراغ visit(Ast_عقدة_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        إذا (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        إذا (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        إذا (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    فراغ visit(Ast_عقدة_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        إذا (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        إذا (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        إذا (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        إذا (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    فراغ visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    فراغ visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قيمة(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قيمة(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_قيمة(nodep->valuep(), nodep->memberp());
    }
    فراغ visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        إذا (nodep->defaultp()) {
            ensure_Cast_قيمة(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    فراغ visit(Ast_تعبير_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    فراغ visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        إذا (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            أدرج_Cast(nodep->lhsp(), castSize(nodep));
        } وإلا {
            إذا (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    فراغ visit(AstVarRef* nodep) override {
        ثابت Ast_عقدة* ثابت backp = nodep->backp();
        إذا (nodep->access().isReadOnly() && VN_IS(backp, عقدة_تعبير) && !VN_IS(backp, CCast)
            && !VN_IS(backp, عقدة_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            أدرج_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    فراغ visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    فراغ visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    فراغ visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(true);
    }
    فراغ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(true);
    }
    فراغ visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(true);
    }
    فراغ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(true);
    }

    // NOPs
    فراغ visit(AstVar*) override {}

    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = افتراضي;
};

//######################################################################
// Cast class functions

فراغ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("cast", 0, dump_شجرة_Either_Level() >= 3);
}
