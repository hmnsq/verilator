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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

ਵਰਗ CastVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE

    // METHODS

    ਖਾਲੀ ਪਾਓ_Cast(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
        //
        AstCCast* ਅਚਲ castp
            = ਨਵਾਂ AstCCast{nodep->fileline(), nodep, needsize, nodep->ਚੌੜਾਈ_Min()};
        UINFO(4, "  MadeCast " << static_cast<ਖਾਲੀ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ਸਥਿਰ ਪੂਰਨ cast_ਮਾਪ(ਪੂਰਨ ਚੌੜਾਈ) {
        ਜੇ (ਚੌੜਾਈ > VL_IDATASIZE && ਚੌੜਾਈ <= VL_QUADSIZE) {
            ਵਾਪਸ VL_QUADSIZE;
        } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ <= 8) {
            ਵਾਪਸ 8;
        } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ <= 16) {
            ਵਾਪਸ 16;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ VL_IDATASIZE;
        }
    }
    ਸਥਿਰ ਪੂਰਨ cast_ਮਾਪ(ਅਚਲ Ast_ਗੰਢ* nodep) { ਵਾਪਸ cast_ਮਾਪ(nodep->ਚੌੜਾਈ()); }
    ਖਾਲੀ ensureCast(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (cast_ਮਾਪ(nodep->backp()) != cast_ਮਾਪ(nodep) || !nodep->user1()) {
            ਜੇ (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ਪਾਓ_Cast(nodep, cast_ਮਾਪ(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    ਖਾਲੀ ensure_Cast_ਮੁੱਲ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਜੇ (dtypep->isIntegralOrPacked() && dtypep->ਚੌੜਾਈ() <= VL_QUADSIZE) {
            ਪਾਓ_Cast(valuep, cast_ਮਾਪ(dtypep->ਚੌੜਾਈ()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    ਖਾਲੀ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        ਜੇ (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ਪਾਓ_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    ਖਾਲੀ ensureNullChecked(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        ਜੇ (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
            relinkHandle.relink(ਨਵਾਂ AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ਜੇ (nodep->ਮਾਪ_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        ਜੇ (nodep->ਮਾਪ_Matters_Lhs()) ensureCast(nodep->lhsp());
        ਜੇ (nodep->ਮਾਪ_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ਅਚਲ AstClassRefDType* ਅਚਲ thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ਅਚਲ AstClassRefDType* ਅਚਲ elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ਅਚਲ ਤਾਰਕਿਕ castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        ਜੇ (castRequired) {
            ਅਚਲ AstClass* ਅਚਲ common_ਅਧਾਰ_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            ਜੇ (thenClassDtypep->classp() != common_ਅਧਾਰ_Classp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* thenp = nodep->thenp()->unlink_Fr_ਪਿੱਠ();
                nodep->thenp(ਨਵਾਂ AstCCast{thenp->fileline(), thenp, nodep});
            }
            ਜੇ (elseClassDtypep->classp() != common_ਅਧਾਰ_Classp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* elsep = nodep->elsep()->unlink_Fr_ਪਿੱਠ();
                nodep->elsep(ਨਵਾਂ AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Triop*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        ਜੇ (nodep->ਮਾਪ_Matters_Lhs()) ensureCast(nodep->lhsp());
        ਜੇ (nodep->ਮਾਪ_Matters_Rhs()) ensureCast(nodep->rhsp());
        ਜੇ (nodep->ਮਾਪ_Matters_Ths()) ensureCast(nodep->thsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        ਜੇ (nodep->ਮਾਪ_Matters_Lhs()) ensureCast(nodep->lhsp());
        ਜੇ (nodep->ਮਾਪ_Matters_Rhs()) ensureCast(nodep->rhsp());
        ਜੇ (nodep->ਮਾਪ_Matters_Ths()) ensureCast(nodep->thsp());
        ਜੇ (nodep->ਮਾਪ_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->defaultp()) {
            ensure_Cast_ਮੁੱਲ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->defaultp()) {
            ensure_Cast_ਮੁੱਲ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cons_Struct_ਵਸਤੂ* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_ਮੁੱਲ(nodep->valuep(), nodep->memberp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ਜੇ (nodep->defaultp()) {
            ensure_Cast_ਮੁੱਲ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ਜੇ (nodep->lhsp()->ਚੌੜਾਈ_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ਪਾਓ_Cast(nodep->lhsp(), cast_ਮਾਪ(nodep));
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->ਮਾਪ_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();
        ਜੇ (nodep->access().isReadOnly() && VN_IS(backp, ਗੰਢ_ਵਿਅੰਜਕ) && !VN_IS(backp, CCast)
            && !VN_IS(backp, ਗੰਢ_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->ਚੌੜਾਈ() && cast_ਮਾਪ(nodep) != cast_ਮਾਪ(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ਪਾਓ_Cast(nodep, cast_ਮਾਪ(nodep));
        }
        nodep->user1(1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    ਖਾਲੀ ਦੇਖੋ(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ਸੱਚ);
    }

    // NOPs
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = ਮੂਲ;
};

//######################################################################
// Cast class functions

ਖਾਲੀ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("cast", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
