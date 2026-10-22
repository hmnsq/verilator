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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

తరగతి CastVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE

    // METHODS

    శూన్య చొప్పించు_Cast(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_వెనుక(&relinkHandle);
        //
        AstCCast* స్థిరరాశి castp
            = కొత్త AstCCast{nodep->fileline(), nodep, needsize, nodep->వెడల్పు_Min()};
        UINFO(4, "  MadeCast " << static_cast<శూన్య*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    స్థిర పూర్ణాంకం cast_కొలత(పూర్ణాంకం వెడల్పు) {
        ఒకవేళ (వెడల్పు > VL_IDATASIZE && వెడల్పు <= VL_QUADSIZE) {
            తిరిగి VL_QUADSIZE;
        } లేకపోతే ఒకవేళ (వెడల్పు <= 8) {
            తిరిగి 8;
        } లేకపోతే ఒకవేళ (వెడల్పు <= 16) {
            తిరిగి 16;
        } లేకపోతే {
            తిరిగి VL_IDATASIZE;
        }
    }
    స్థిర పూర్ణాంకం cast_కొలత(స్థిరరాశి Ast_కణుపు* nodep) { తిరిగి cast_కొలత(nodep->వెడల్పు()); }
    శూన్య ensureCast(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (cast_కొలత(nodep->backp()) != cast_కొలత(nodep) || !nodep->user1()) {
            ఒకవేళ (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                చొప్పించు_Cast(nodep, cast_కొలత(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    శూన్య ensure_Cast_విలువ(Ast_కణుపు_సమాసం* valuep, స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (dtypep->isIntegralOrPacked() && dtypep->వెడల్పు() <= VL_QUADSIZE) {
            చొప్పించు_Cast(valuep, cast_కొలత(dtypep->వెడల్పు()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    శూన్య ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        ఒకవేళ (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            చొప్పించు_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    శూన్య ensureNullChecked(Ast_కణుపు_సమాసం* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        ఒకవేళ (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_వెనుక(&relinkHandle);
            relinkHandle.relink(కొత్త AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ఒకవేళ (nodep->కొలత_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        ఒకవేళ (nodep->కొలత_Matters_Lhs()) ensureCast(nodep->lhsp());
        ఒకవేళ (nodep->కొలత_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        స్థిరరాశి AstClassRefDType* స్థిరరాశి thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        స్థిరరాశి AstClassRefDType* స్థిరరాశి elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        స్థిరరాశి తార్కిక castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        ఒకవేళ (castRequired) {
            స్థిరరాశి AstClass* స్థిరరాశి common_పునాది_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            ఒకవేళ (thenClassDtypep->classp() != common_పునాది_Classp) {
                Ast_కణుపు_సమాసం* thenp = nodep->thenp()->unlink_Fr_వెనుక();
                nodep->thenp(కొత్త AstCCast{thenp->fileline(), thenp, nodep});
            }
            ఒకవేళ (elseClassDtypep->classp() != common_పునాది_Classp) {
                Ast_కణుపు_సమాసం* elsep = nodep->elsep()->unlink_Fr_వెనుక();
                nodep->elsep(కొత్త AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        సందర్శించు(static_cast<Ast_కణుపు_Triop*>(nodep));
    }
    శూన్య సందర్శించు(Ast_కణుపు_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        ఒకవేళ (nodep->కొలత_Matters_Lhs()) ensureCast(nodep->lhsp());
        ఒకవేళ (nodep->కొలత_Matters_Rhs()) ensureCast(nodep->rhsp());
        ఒకవేళ (nodep->కొలత_Matters_Ths()) ensureCast(nodep->thsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        ఒకవేళ (nodep->కొలత_Matters_Lhs()) ensureCast(nodep->lhsp());
        ఒకవేళ (nodep->కొలత_Matters_Rhs()) ensureCast(nodep->rhsp());
        ఒకవేళ (nodep->కొలత_Matters_Ths()) ensureCast(nodep->thsp());
        ఒకవేళ (nodep->కొలత_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    శూన్య సందర్శించు(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    శూన్య సందర్శించు(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->defaultp()) {
            ensure_Cast_విలువ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    శూన్య సందర్శించు(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->defaultp()) {
            ensure_Cast_విలువ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    శూన్య సందర్శించు(Ast_Cons_Struct_అంశం* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_విలువ(nodep->valuep(), nodep->memberp());
    }
    శూన్య సందర్శించు(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->defaultp()) {
            ensure_Cast_విలువ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    శూన్య సందర్శించు(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ఒకవేళ (nodep->lhsp()->వెడల్పు_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            చొప్పించు_Cast(nodep->lhsp(), cast_కొలత(nodep));
        } లేకపోతే {
            ఒకవేళ (nodep->కొలత_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();
        ఒకవేళ (nodep->access().isReadOnly() && VN_IS(backp, కణుపు_సమాసం) && !VN_IS(backp, CCast)
            && !VN_IS(backp, కణుపు_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->వెడల్పు() && cast_కొలత(nodep) != cast_కొలత(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            చొప్పించు_Cast(nodep, cast_కొలత(nodep));
        }
        nodep->user1(1);
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    శూన్య సందర్శించు(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    శూన్య సందర్శించు(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(నిజం);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(నిజం);
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(నిజం);
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(నిజం);
    }

    // NOPs
    శూన్య సందర్శించు(AstVar*) override {}

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = అప్రమేయ;
};

//######################################################################
// Cast class functions

శూన్య V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("cast", 0, dump_వృక్షం_Either_Level() >= 3);
}
