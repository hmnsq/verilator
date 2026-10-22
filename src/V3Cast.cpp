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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

श्रेणी CastVisitor final : खुला VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    स्थिर VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    व्योम जोड़ो_Cast(Ast_गाँठ_व्यंजक* nodep, पूर्णांक needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* स्थिर castp
            = नया AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<व्योम*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    अटल पूर्णांक cast_आकार(पूर्णांक width) {
        अगर (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            वापस VL_QUADSIZE;
        } अन्यथा अगर (width <= 8) {
            वापस 8;
        } अन्यथा अगर (width <= 16) {
            वापस 16;
        } अन्यथा {
            वापस VL_IDATASIZE;
        }
    }
    अटल पूर्णांक cast_आकार(स्थिर Ast_गाँठ* nodep) { वापस cast_आकार(nodep->width()); }
    व्योम ensureCast(Ast_गाँठ_व्यंजक* nodep) {
        अगर (cast_आकार(nodep->backp()) != cast_आकार(nodep) || !nodep->user1()) {
            अगर (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                जोड़ो_Cast(nodep, cast_आकार(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    व्योम ensure_Cast_मूल्य(Ast_गाँठ_व्यंजक* valuep, स्थिर Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        अगर (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            जोड़ो_Cast(valuep, cast_आकार(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    व्योम ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        अगर (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            जोड़ो_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    व्योम ensureNullChecked(Ast_गाँठ_व्यंजक* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        अगर (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(नया AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        अगर (nodep->आकार_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    व्योम visit(Ast_गाँठ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        अगर (nodep->आकार_Matters_Lhs()) ensureCast(nodep->lhsp());
        अगर (nodep->आकार_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    व्योम visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        स्थिर AstClassRefDType* स्थिर thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        स्थिर AstClassRefDType* स्थिर elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        स्थिर bool castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        अगर (castRequired) {
            स्थिर AstClass* स्थिर commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            अगर (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_गाँठ_व्यंजक* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(नया AstCCast{thenp->fileline(), thenp, nodep});
            }
            अगर (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_गाँठ_व्यंजक* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(नया AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_गाँठ_Triop*>(nodep));
    }
    व्योम visit(Ast_गाँठ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        अगर (nodep->आकार_Matters_Lhs()) ensureCast(nodep->lhsp());
        अगर (nodep->आकार_Matters_Rhs()) ensureCast(nodep->rhsp());
        अगर (nodep->आकार_Matters_Ths()) ensureCast(nodep->thsp());
    }
    व्योम visit(Ast_गाँठ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        अगर (nodep->आकार_Matters_Lhs()) ensureCast(nodep->lhsp());
        अगर (nodep->आकार_Matters_Rhs()) ensureCast(nodep->rhsp());
        अगर (nodep->आकार_Matters_Ths()) ensureCast(nodep->thsp());
        अगर (nodep->आकार_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    व्योम visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    व्योम visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->defaultp()) {
            ensure_Cast_मूल्य(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    व्योम visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->defaultp()) {
            ensure_Cast_मूल्य(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    व्योम visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_मूल्य(nodep->valuep(), nodep->memberp());
    }
    व्योम visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->defaultp()) {
            ensure_Cast_मूल्य(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    व्योम visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        अगर (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            जोड़ो_Cast(nodep->lhsp(), cast_आकार(nodep));
        } अन्यथा {
            अगर (nodep->आकार_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    व्योम visit(AstVarRef* nodep) override {
        स्थिर Ast_गाँठ* स्थिर backp = nodep->backp();
        अगर (nodep->access().isReadOnly() && VN_IS(backp, गाँठ_व्यंजक) && !VN_IS(backp, CCast)
            && !VN_IS(backp, गाँठ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && cast_आकार(nodep) != cast_आकार(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            जोड़ो_Cast(nodep, cast_आकार(nodep));
        }
        nodep->user1(1);
    }
    व्योम visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    व्योम visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    व्योम visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(true);
    }
    व्योम visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(true);
    }
    व्योम visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(true);
    }
    व्योम visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(true);
    }

    // NOPs
    व्योम visit(AstVar*) override {}

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = शेष;
};

//######################################################################
// Cast class functions

व्योम V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("cast", 0, dump_वृक्ष_Either_Level() >= 3);
}
