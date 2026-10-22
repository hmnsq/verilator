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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

वर्ग CastVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ध्रुव VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    शून्य जोड़ो_Cast(Ast_गाँठ_व्यंजक* nodep, पूर्णाङ्क needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* ध्रुव castp
            = नूतन AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<शून्य*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    स्थिर पूर्णाङ्क castSize(पूर्णाङ्क width) {
        यदि (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            प्रत्यागम VL_QUADSIZE;
        } अन्यथा यदि (width <= 8) {
            प्रत्यागम 8;
        } अन्यथा यदि (width <= 16) {
            प्रत्यागम 16;
        } अन्यथा {
            प्रत्यागम VL_IDATASIZE;
        }
    }
    स्थिर पूर्णाङ्क castSize(ध्रुव Ast_गाँठ* nodep) { प्रत्यागम castSize(nodep->width()); }
    शून्य ensureCast(Ast_गाँठ_व्यंजक* nodep) {
        यदि (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            यदि (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                जोड़ो_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    शून्य ensure_Cast_मूल्य(Ast_गाँठ_व्यंजक* valuep, ध्रुव Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            जोड़ो_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    शून्य ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        यदि (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            जोड़ो_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    शून्य ensureNullChecked(Ast_गाँठ_व्यंजक* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        यदि (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(नूतन AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    शून्य visit(Ast_गाँठ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        यदि (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    शून्य visit(Ast_गाँठ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        यदि (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        यदि (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    शून्य visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ध्रुव AstClassRefDType* ध्रुव thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ध्रुव AstClassRefDType* ध्रुव elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ध्रुव तार्किक castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        यदि (castRequired) {
            ध्रुव AstClass* ध्रुव commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            यदि (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_गाँठ_व्यंजक* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(नूतन AstCCast{thenp->fileline(), thenp, nodep});
            }
            यदि (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_गाँठ_व्यंजक* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(नूतन AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_गाँठ_Triop*>(nodep));
    }
    शून्य visit(Ast_गाँठ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        यदि (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        यदि (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        यदि (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    शून्य visit(Ast_गाँठ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        यदि (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        यदि (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        यदि (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        यदि (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    शून्य visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    शून्य visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) {
            ensure_Cast_मूल्य(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    शून्य visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) {
            ensure_Cast_मूल्य(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    शून्य visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_मूल्य(nodep->valuep(), nodep->memberp());
    }
    शून्य visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) {
            ensure_Cast_मूल्य(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    शून्य visit(Ast_व्यंजक_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    शून्य visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        यदि (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            जोड़ो_Cast(nodep->lhsp(), castSize(nodep));
        } अन्यथा {
            यदि (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    शून्य visit(AstVarRef* nodep) override {
        ध्रुव Ast_गाँठ* ध्रुव backp = nodep->backp();
        यदि (nodep->access().isReadOnly() && VN_IS(backp, गाँठ_व्यंजक) && !VN_IS(backp, CCast)
            && !VN_IS(backp, गाँठ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            जोड़ो_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    शून्य visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    शून्य visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    शून्य visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(सत्य);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(सत्य);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(सत्य);
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(सत्य);
    }

    // NOPs
    शून्य visit(AstVar*) override {}

    //--------------------
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = स्वभाव;
};

//######################################################################
// Cast class functions

शून्य V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("cast", 0, dump_वृक्ष_Either_Level() >= 3);
}
