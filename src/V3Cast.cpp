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

رده CastVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ثابت VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    تهی درج_Cast(Ast_گره_عبارت* nodep, صحیح needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* ثابت castp
            = جدید AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<تهی*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ایستا صحیح castSize(صحیح width) {
        اگر (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            بازگشت VL_QUADSIZE;
        } وگرنه اگر (width <= 8) {
            بازگشت 8;
        } وگرنه اگر (width <= 16) {
            بازگشت 16;
        } وگرنه {
            بازگشت VL_IDATASIZE;
        }
    }
    ایستا صحیح castSize(ثابت Ast_گره* nodep) { بازگشت castSize(nodep->width()); }
    تهی ensureCast(Ast_گره_عبارت* nodep) {
        اگر (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            اگر (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                درج_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    تهی ensure_Cast_مقدار(Ast_گره_عبارت* valuep, ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            درج_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    تهی ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        اگر (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            درج_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    تهی ensureNullChecked(Ast_گره_عبارت* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        اگر (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(جدید AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    تهی visit(Ast_گره_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        اگر (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    تهی visit(Ast_گره_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        اگر (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        اگر (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    تهی visit(AstCond* nodep) override {
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
            ثابت AstClass* ثابت commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            اگر (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_گره_عبارت* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(جدید AstCCast{thenp->fileline(), thenp, nodep});
            }
            اگر (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_گره_عبارت* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(جدید AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_گره_Triop*>(nodep));
    }
    تهی visit(Ast_گره_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        اگر (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        اگر (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        اگر (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    تهی visit(Ast_گره_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        اگر (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        اگر (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        اگر (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        اگر (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    تهی visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    تهی visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) {
            ensure_Cast_مقدار(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    تهی visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) {
            ensure_Cast_مقدار(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    تهی visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_مقدار(nodep->valuep(), nodep->memberp());
    }
    تهی visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        اگر (nodep->defaultp()) {
            ensure_Cast_مقدار(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    تهی visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        اگر (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            درج_Cast(nodep->lhsp(), castSize(nodep));
        } وگرنه {
            اگر (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    تهی visit(AstVarRef* nodep) override {
        ثابت Ast_گره* ثابت backp = nodep->backp();
        اگر (nodep->access().isReadOnly() && VN_IS(backp, گره_عبارت) && !VN_IS(backp, CCast)
            && !VN_IS(backp, گره_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            درج_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    تهی visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    تهی visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    تهی visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(درست);
    }
    تهی visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(درست);
    }
    تهی visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(درست);
    }
    تهی visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(درست);
    }

    // NOPs
    تهی visit(AstVar*) override {}

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = پیشفرض;
};

//######################################################################
// Cast class functions

تهی V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("cast", 0, dump_درخت_Either_Level() >= 3);
}
