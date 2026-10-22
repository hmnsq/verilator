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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Cast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Cast state, as a visitor of each AstNode

ವರ್ಗ CastVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //   AstNode::user1()               // bool.  Node is of known size
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE

    // METHODS

    ಶೂನ್ಯ ಸೇರಿಸು_Cast(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಂಕ needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
        //
        AstCCast* ಸ್ಥಿರಾಂಕ castp
            = ಹೊಸ AstCCast{nodep->fileline(), nodep, needsize, nodep->ಅಗಲ_Min()};
        UINFO(4, "  MadeCast " << static_cast<ಶೂನ್ಯ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ cast_ಗಾತ್ರ(ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        ಆದರೆ (ಅಗಲ > VL_IDATASIZE && ಅಗಲ <= VL_QUADSIZE) {
            ಹಿಂತಿರುಗಿ VL_QUADSIZE;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ <= 8) {
            ಹಿಂತಿರುಗಿ 8;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ <= 16) {
            ಹಿಂತಿರುಗಿ 16;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ VL_IDATASIZE;
        }
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ cast_ಗಾತ್ರ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ cast_ಗಾತ್ರ(nodep->ಅಗಲ()); }
    ಶೂನ್ಯ ensureCast(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (cast_ಗಾತ್ರ(nodep->backp()) != cast_ಗಾತ್ರ(nodep) || !nodep->user1()) {
            ಆದರೆ (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ಸೇರಿಸು_Cast(nodep, cast_ಗಾತ್ರ(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    ಶೂನ್ಯ ensure_Cast_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (dtypep->isIntegralOrPacked() && dtypep->ಅಗಲ() <= VL_QUADSIZE) {
            ಸೇರಿಸು_Cast(valuep, cast_ಗಾತ್ರ(dtypep->ಅಗಲ()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    ಶೂನ್ಯ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        ಆದರೆ (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ಸೇರಿಸು_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    ಶೂನ್ಯ ensureNullChecked(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        ಆದರೆ (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
            relinkHandle.relink(ಹೊಸ AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Lhs()) ensureCast(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Lhs()) ensureCast(nodep->lhsp());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Rhs()) ensureCast(nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        ಆದರೆ (castRequired) {
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ common_ತಳ_Classp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            ಆದರೆ (thenClassDtypep->classp() != common_ತಳ_Classp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thenp = nodep->thenp()->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->thenp(ಹೊಸ AstCCast{thenp->fileline(), thenp, nodep});
            }
            ಆದರೆ (elseClassDtypep->classp() != common_ತಳ_Classp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* elsep = nodep->elsep()->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->elsep(ಹೊಸ AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Triop*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Lhs()) ensureCast(nodep->lhsp());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Rhs()) ensureCast(nodep->rhsp());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Ths()) ensureCast(nodep->thsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Lhs()) ensureCast(nodep->lhsp());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Rhs()) ensureCast(nodep->rhsp());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Ths()) ensureCast(nodep->thsp());
        ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Fhs()) ensureCast(nodep->fhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->defaultp()) {
            ensure_Cast_ಮೌಲ್ಯ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->defaultp()) {
            ensure_Cast_ಮೌಲ್ಯ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cons_Struct_ಅಂಶ* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_ಮೌಲ್ಯ(nodep->valuep(), nodep->memberp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (nodep->defaultp()) {
            ensure_Cast_ಮೌಲ್ಯ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ಆದರೆ (nodep->lhsp()->ಅಗಲ_Min() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ಸೇರಿಸು_Cast(nodep->lhsp(), cast_ಗಾತ್ರ(nodep));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->ಗಾತ್ರ_Matters_Lhs()) ensureCast(nodep->lhsp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();
        ಆದರೆ (nodep->access().isReadOnly() && VN_IS(backp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) && !VN_IS(backp, CCast)
            && !VN_IS(backp, ಗಂಟು_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->ಅಗಲ() && cast_ಗಾತ್ರ(nodep) != cast_ಗಾತ್ರ(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ಸೇರಿಸು_Cast(nodep, cast_ಗಾತ್ರ(nodep));
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    ಶೂನ್ಯ ಭೇಟಿ(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ನಿಜ);
    }

    // NOPs
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Cast class functions

ಶೂನ್ಯ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cast", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
