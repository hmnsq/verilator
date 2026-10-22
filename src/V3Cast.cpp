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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

ଶ୍ରେଣୀ CastVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Cast(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* ଅଚଳ castp
            = ନୂତନ AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<ଶୂନ୍ୟ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା castSize(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        ଯଦି (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            ଫେରନ୍ତୁ VL_QUADSIZE;
        } ଅନ୍ୟଥା ଯଦି (width <= 8) {
            ଫେରନ୍ତୁ 8;
        } ଅନ୍ୟଥା ଯଦି (width <= 16) {
            ଫେରନ୍ତୁ 16;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ VL_IDATASIZE;
        }
    }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା castSize(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ castSize(nodep->width()); }
    ଶୂନ୍ୟ ensureCast(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            ଯଦି (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ଭର୍ତ୍ତି_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    ଶୂନ୍ୟ ensure_Cast_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଯଦି (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            ଭର୍ତ୍ତି_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    ଶୂନ୍ୟ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        ଯଦି (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ଭର୍ତ୍ତି_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    ଶୂନ୍ୟ ensureNullChecked(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        ଯଦି (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(ନୂତନ AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ଯଦି (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        ଯଦି (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        ଯଦି (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ଅଚଳ AstClassRefDType* ଅଚଳ thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ଅଚଳ AstClassRefDType* ଅଚଳ elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ଅଚଳ ତାର୍କିକ castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        ଯଦି (castRequired) {
            ଅଚଳ AstClass* ଅଚଳ commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            ଯଦି (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(ନୂତନ AstCCast{thenp->fileline(), thenp, nodep});
            }
            ଯଦି (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(ନୂତନ AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_ଗଣ୍ଠି_Triop*>(nodep));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        ଯଦି (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        ଯଦି (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        ଯଦି (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        ଯଦି (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        ଯଦି (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        ଯଦି (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        ଯଦି (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    ଶୂନ୍ୟ visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    ଶୂନ୍ୟ visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->defaultp()) {
            ensure_Cast_ମୂଲ୍ୟ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ଶୂନ୍ୟ visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->defaultp()) {
            ensure_Cast_ମୂଲ୍ୟ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ଶୂନ୍ୟ visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_ମୂଲ୍ୟ(nodep->valuep(), nodep->memberp());
    }
    ଶୂନ୍ୟ visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ଯଦି (nodep->defaultp()) {
            ensure_Cast_ମୂଲ୍ୟ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    ଶୂନ୍ୟ visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ଯଦି (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ଭର୍ତ୍ତି_Cast(nodep->lhsp(), castSize(nodep));
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();
        ଯଦି (nodep->access().isReadOnly() && VN_IS(backp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି) && !VN_IS(backp, CCast)
            && !VN_IS(backp, ଗଣ୍ଠି_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ଭର୍ତ୍ତି_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    ଶୂନ୍ୟ visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    ଶୂନ୍ୟ visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ସତ୍ୟ);
    }

    // NOPs
    ଶୂନ୍ୟ visit(AstVar*) override {}

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Cast class functions

ଶୂନ୍ୟ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("cast", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
