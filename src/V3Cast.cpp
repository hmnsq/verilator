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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

வகுப்பு CastVisitor final : பொது VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    மாறிலி VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    வெறுமை செருகு_Cast(Ast_முனை_கோவை* nodep, முழு needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* மாறிலி castp
            = புதிய AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<வெறுமை*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    நிலையான முழு castSize(முழு width) {
        எனில் (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            திரும்பு VL_QUADSIZE;
        } இல்லையெனில் எனில் (width <= 8) {
            திரும்பு 8;
        } இல்லையெனில் எனில் (width <= 16) {
            திரும்பு 16;
        } இல்லையெனில் {
            திரும்பு VL_IDATASIZE;
        }
    }
    நிலையான முழு castSize(மாறிலி Ast_முனை* nodep) { திரும்பு castSize(nodep->width()); }
    வெறுமை ensureCast(Ast_முனை_கோவை* nodep) {
        எனில் (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            எனில் (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                செருகு_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    வெறுமை ensure_Cast_மதிப்பு(Ast_முனை_கோவை* valuep, மாறிலி Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        எனில் (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            செருகு_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    வெறுமை ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        எனில் (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            செருகு_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    வெறுமை ensureNullChecked(Ast_முனை_கோவை* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        எனில் (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(புதிய AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        எனில் (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        எனில் (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        எனில் (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    வெறுமை visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        மாறிலி AstClassRefDType* மாறிலி thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        மாறிலி AstClassRefDType* மாறிலி elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        மாறிலி தருக்க castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        எனில் (castRequired) {
            மாறிலி AstClass* மாறிலி commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            எனில் (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_முனை_கோவை* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(புதிய AstCCast{thenp->fileline(), thenp, nodep});
            }
            எனில் (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_முனை_கோவை* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(புதிய AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_முனை_Triop*>(nodep));
    }
    வெறுமை visit(Ast_முனை_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        எனில் (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        எனில் (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        எனில் (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    வெறுமை visit(Ast_முனை_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        எனில் (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        எனில் (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        எனில் (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        எனில் (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    வெறுமை visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    வெறுமை visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->defaultp()) {
            ensure_Cast_மதிப்பு(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    வெறுமை visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->defaultp()) {
            ensure_Cast_மதிப்பு(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    வெறுமை visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_மதிப்பு(nodep->valuep(), nodep->memberp());
    }
    வெறுமை visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        எனில் (nodep->defaultp()) {
            ensure_Cast_மதிப்பு(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    வெறுமை visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        எனில் (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            செருகு_Cast(nodep->lhsp(), castSize(nodep));
        } இல்லையெனில் {
            எனில் (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    வெறுமை visit(AstVarRef* nodep) override {
        மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();
        எனில் (nodep->access().isReadOnly() && VN_IS(backp, முனை_கோவை) && !VN_IS(backp, CCast)
            && !VN_IS(backp, முனை_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            செருகு_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    வெறுமை visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    வெறுமை visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    வெறுமை visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(உண்மை);
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(உண்மை);
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(உண்மை);
    }
    வெறுமை visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(உண்மை);
    }

    // NOPs
    வெறுமை visit(AstVar*) override {}

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = இயல்பு;
};

//######################################################################
// Cast class functions

வெறுமை V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("cast", 0, dump_மரம்_Either_Level() >= 3);
}
