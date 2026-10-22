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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

વર્ગ CastVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    અચલ VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    શૂન્ય ઉમેરો_Cast(Ast_ગાંઠ_પદાવલિ* nodep, પૂર્ણાંક needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* અચલ castp
            = નવું AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<શૂન્ય*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    સ્થિર પૂર્ણાંક castSize(પૂર્ણાંક width) {
        જો (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            પાછા VL_QUADSIZE;
        } નહીંતર જો (width <= 8) {
            પાછા 8;
        } નહીંતર જો (width <= 16) {
            પાછા 16;
        } નહીંતર {
            પાછા VL_IDATASIZE;
        }
    }
    સ્થિર પૂર્ણાંક castSize(અચલ Ast_ગાંઠ* nodep) { પાછા castSize(nodep->width()); }
    શૂન્ય ensureCast(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            જો (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ઉમેરો_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    શૂન્ય ensure_Cast_કિંમત(Ast_ગાંઠ_પદાવલિ* valuep, અચલ Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        જો (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            ઉમેરો_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    શૂન્ય ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        જો (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ઉમેરો_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    શૂન્ય ensureNullChecked(Ast_ગાંઠ_પદાવલિ* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        જો (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(નવું AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        જો (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        જો (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        જો (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    શૂન્ય visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        અચલ AstClassRefDType* અચલ thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        અચલ AstClassRefDType* અચલ elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        અચલ તાર્કિક castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        જો (castRequired) {
            અચલ AstClass* અચલ commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            જો (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_ગાંઠ_પદાવલિ* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(નવું AstCCast{thenp->fileline(), thenp, nodep});
            }
            જો (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_ગાંઠ_પદાવલિ* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(નવું AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_ગાંઠ_Triop*>(nodep));
    }
    શૂન્ય visit(Ast_ગાંઠ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        જો (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        જો (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        જો (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        જો (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        જો (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        જો (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        જો (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    શૂન્ય visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    શૂન્ય visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        જો (nodep->defaultp()) {
            ensure_Cast_કિંમત(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    શૂન્ય visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        જો (nodep->defaultp()) {
            ensure_Cast_કિંમત(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    શૂન્ય visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_કિંમત(nodep->valuep(), nodep->memberp());
    }
    શૂન્ય visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        જો (nodep->defaultp()) {
            ensure_Cast_કિંમત(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    શૂન્ય visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        જો (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ઉમેરો_Cast(nodep->lhsp(), castSize(nodep));
        } નહીંતર {
            જો (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        અચલ Ast_ગાંઠ* અચલ backp = nodep->backp();
        જો (nodep->access().isReadOnly() && VN_IS(backp, ગાંઠ_પદાવલિ) && !VN_IS(backp, CCast)
            && !VN_IS(backp, ગાંઠ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ઉમેરો_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    શૂન્ય visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    શૂન્ય visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    શૂન્ય visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(સાચું);
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(સાચું);
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(સાચું);
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(સાચું);
    }

    // NOPs
    શૂન્ય visit(AstVar*) override {}

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = મૂળભૂત;
};

//######################################################################
// Cast class functions

શૂન્ય V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("cast", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
