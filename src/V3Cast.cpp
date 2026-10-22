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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

വർഗ്ഗം CastVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    ശൂന്യം തിരുകുക_Cast(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* സ്ഥിരം castp
            = പുതിയ AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<ശൂന്യം*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    സ്ഥിതം പൂർണ്ണസംഖ്യ castSize(പൂർണ്ണസംഖ്യ width) {
        എങ്കിൽ (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            തിരികെ VL_QUADSIZE;
        } അല്ലെങ്കിൽ എങ്കിൽ (width <= 8) {
            തിരികെ 8;
        } അല്ലെങ്കിൽ എങ്കിൽ (width <= 16) {
            തിരികെ 16;
        } അല്ലെങ്കിൽ {
            തിരികെ VL_IDATASIZE;
        }
    }
    സ്ഥിതം പൂർണ്ണസംഖ്യ castSize(സ്ഥിരം Ast_കണ്ണി* nodep) { തിരികെ castSize(nodep->width()); }
    ശൂന്യം ensureCast(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            എങ്കിൽ (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                തിരുകുക_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    ശൂന്യം ensure_Cast_മൂല്യം(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            തിരുകുക_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    ശൂന്യം ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        എങ്കിൽ (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            തിരുകുക_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    ശൂന്യം ensureNullChecked(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        എങ്കിൽ (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(പുതിയ AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        എങ്കിൽ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        എങ്കിൽ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        എങ്കിൽ (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    ശൂന്യം visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        സ്ഥിരം AstClassRefDType* സ്ഥിരം thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        സ്ഥിരം AstClassRefDType* സ്ഥിരം elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        സ്ഥിരം തർക്കം castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        എങ്കിൽ (castRequired) {
            സ്ഥിരം AstClass* സ്ഥിരം commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            എങ്കിൽ (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(പുതിയ AstCCast{thenp->fileline(), thenp, nodep});
            }
            എങ്കിൽ (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(പുതിയ AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_കണ്ണി_Triop*>(nodep));
    }
    ശൂന്യം visit(Ast_കണ്ണി_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        എങ്കിൽ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        എങ്കിൽ (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        എങ്കിൽ (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        എങ്കിൽ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        എങ്കിൽ (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        എങ്കിൽ (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        എങ്കിൽ (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    ശൂന്യം visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    ശൂന്യം visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->defaultp()) {
            ensure_Cast_മൂല്യം(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ശൂന്യം visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->defaultp()) {
            ensure_Cast_മൂല്യം(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ശൂന്യം visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_മൂല്യം(nodep->valuep(), nodep->memberp());
    }
    ശൂന്യം visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->defaultp()) {
            ensure_Cast_മൂല്യം(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    ശൂന്യം visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        എങ്കിൽ (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            തിരുകുക_Cast(nodep->lhsp(), castSize(nodep));
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();
        എങ്കിൽ (nodep->access().isReadOnly() && VN_IS(backp, കണ്ണി_വ്യഞ്ജകം) && !VN_IS(backp, CCast)
            && !VN_IS(backp, കണ്ണി_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            തിരുകുക_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    ശൂന്യം visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    ശൂന്യം visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    ശൂന്യം visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(സത്യം);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(സത്യം);
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(സത്യം);
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(സത്യം);
    }

    // NOPs
    ശൂന്യം visit(AstVar*) override {}

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Cast class functions

ശൂന്യം V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("cast", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
