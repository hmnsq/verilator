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
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE

    // METHODS

    ಶೂನ್ಯ ಸೇರಿಸು_Cast(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ needsize) {  // We'll insert ABOVE passed node
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        AstCCast* ಧ್ರುವ castp
            = ನೂತನ AstCCast{nodep->fileline(), nodep, needsize, nodep->widthMin()};
        UINFO(4, "  MadeCast " << static_cast<ಶೂನ್ಯ*>(castp) << " for " << nodep);
        relinkHandle.relink(castp);
        // UINFOTREE(9, castp, "", "castins");
        //
        ensureLower32Cast(castp);
        nodep->user1(1);  // Now must be of known size
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ castSize(ಪೂರ್ಣಾಙ್ಕ width) {
        ಯದಿ (width > VL_IDATASIZE && width <= VL_QUADSIZE) {
            ಪ್ರತ್ಯಾಗಮ VL_QUADSIZE;
        } ಅನ್ಯಥಾ ಯದಿ (width <= 8) {
            ಪ್ರತ್ಯಾಗಮ 8;
        } ಅನ್ಯಥಾ ಯದಿ (width <= 16) {
            ಪ್ರತ್ಯಾಗಮ 16;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ VL_IDATASIZE;
        }
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ castSize(ಧ್ರುವ Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ castSize(nodep->width()); }
    ಶೂನ್ಯ ensureCast(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (castSize(nodep->backp()) != castSize(nodep) || !nodep->user1()) {
            ಯದಿ (!nodep->isNull() && !nodep->isString() && !nodep->isDouble())
                ಸೇರಿಸು_Cast(nodep, castSize(nodep->backp()));
        }
    }
    // Cast a constructor value to the C type of the element or member 'dtypep' it is stored in,
    // as V3Clean widened the value to its C++ width
    ಶೂನ್ಯ ensure_Cast_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (dtypep->isIntegralOrPacked() && dtypep->width() <= VL_QUADSIZE) {
            ಸೇರಿಸು_Cast(valuep, castSize(dtypep->width()));
        }
    }
    // cppcheck-suppress constParameterPointer // lhsp might be changed
    ಶೂನ್ಯ ensureLower32Cast(AstCCast* nodep) {
        // If we have uint64 = CAST(uint64(x)) then the upcasting
        // really needs to be CAST(uint64(CAST(uint32(x))).
        // Otherwise a (uint64)(a>b) would return wrong value, as
        // less than has nondeterministic signedness.
        ಯದಿ (nodep->isQuad() && !nodep->lhsp()->isQuad() && !VN_IS(nodep->lhsp(), CCast)) {
            ಸೇರಿಸು_Cast(nodep->lhsp(), VL_IDATASIZE);
        }
    }
    ಶೂನ್ಯ ensureNullChecked(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // TODO optimize to track null checked values and avoid where possible
        ಯದಿ (!VN_IS(nodep->backp(), NullCheck)) {
            VNRelinker relinkHandle;
            nodep->unlinkFrBack(&relinkHandle);
            relinkHandle.relink(ನೂತನ AstNullCheck{nodep->fileline(), nodep});
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ಯದಿ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1());
        ಯದಿ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        ಯದಿ (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        // All class types are castable to each other. If they are of different types,
        // a compilation error will be thrown, so an explicit cast is required. Types were
        // already checked by V3Width and dtypep of a condition operator is a type of their
        // common base class, so both classes can be safely casted.
        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ thenClassDtypep
            = VN_CAST(nodep->thenp()->dtypep()->skipRefp(), ClassRefDType);
        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ elseClassDtypep
            = VN_CAST(nodep->elsep()->dtypep()->skipRefp(), ClassRefDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ castRequired = thenClassDtypep && elseClassDtypep
                                  && (thenClassDtypep->classp() != elseClassDtypep->classp());
        ಯದಿ (castRequired) {
            ಧ್ರುವ AstClass* ಧ್ರುವ commonBaseClassp
                = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType)->classp();
            ಯದಿ (thenClassDtypep->classp() != commonBaseClassp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thenp = nodep->thenp()->unlinkFrBack();
                nodep->thenp(ನೂತನ AstCCast{thenp->fileline(), thenp, nodep});
            }
            ಯದಿ (elseClassDtypep->classp() != commonBaseClassp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* elsep = nodep->elsep()->unlinkFrBack();
                nodep->elsep(ನೂತನ AstCCast{elsep->fileline(), elsep, nodep});
            }
        }
        visit(static_cast<Ast_ಗಂಟು_Triop*>(nodep));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Triop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1());
        ಯದಿ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        ಯದಿ (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        ಯದಿ (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Quadop* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1() | nodep->rhsp()->user1() | nodep->thsp()->user1()
                     | nodep->fhsp()->user1());
        ಯದಿ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        ಯದಿ (nodep->sizeMattersRhs()) ensureCast(nodep->rhsp());
        ಯದಿ (nodep->sizeMattersThs()) ensureCast(nodep->thsp());
        ಯದಿ (nodep->sizeMattersFhs()) ensureCast(nodep->fhsp());
    }
    ಶೂನ್ಯ visit(AstCCast* nodep) override {
        iterateChildren(nodep);
        ensureLower32Cast(nodep);
        nodep->user1(1);
    }
    ಶೂನ್ಯ visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->defaultp()) {
            ensure_Cast_ಮೌಲ್ಯ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->defaultp()) {
            ensure_Cast_ಮೌಲ್ಯ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ visit(AstConsStructItem* nodep) override {
        iterateChildren(nodep);
        ensure_Cast_ಮೌಲ್ಯ(nodep->valuep(), nodep->memberp());
    }
    ಶೂನ್ಯ visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (nodep->defaultp()) {
            ensure_Cast_ಮೌಲ್ಯ(nodep->defaultp(), nodep->dtypep()->skipRefp()->subDTypep());
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        iterateChildren(nodep);
        nodep->user1(1);
    }
    ಶೂನ್ಯ visit(AstNegate* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
        ಯದಿ (nodep->lhsp()->widthMin() == 1 && !nodep->lhsp()->isWide()) {
            // We want to avoid a GCC "converting of negative value" warning
            // from our expansion of
            //    out = {32{a<b}}  =>   out = - (a<b)
            ಸೇರಿಸು_Cast(nodep->lhsp(), castSize(nodep));
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->sizeMattersLhs()) ensureCast(nodep->lhsp());
        }
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();
        ಯದಿ (nodep->access().isReadOnly() && VN_IS(backp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) && !VN_IS(backp, CCast)
            && !VN_IS(backp, ಗಂಟು_CCall) && !VN_IS(backp, CMethodHard) && !VN_IS(backp, SFormatF)
            && !VN_IS(backp, ArraySel) && !VN_IS(backp, StructSel) && !VN_IS(backp, RedXor)
            && (nodep->varp()->basicp() && !nodep->varp()->basicp()->isForkSync()
                && !nodep->varp()->basicp()->isProcessRef() && !nodep->varp()->basicp()->isEvent())
            && backp->width() && castSize(nodep) != castSize(nodep->varp())) {
            // Cast vars to IData first, else below has upper bits wrongly set
            //  CData x=3; out = (QData)(x<<30);
            ಸೇರಿಸು_Cast(nodep, castSize(nodep));
        }
        nodep->user1(1);
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        // Constants are of unknown size if smaller than 33 bits, because
        // we're too lazy to wrap every constant in the universe in
        // ((IData)#).
        nodep->user1(nodep->isQuad() || nodep->isWide() || nodep->isNull());
    }

    // Null dereference protection
    ಶೂನ್ಯ visit(AstNullCheck* nodep) override {
        iterateChildren(nodep);
        nodep->user1(nodep->lhsp()->user1());
    }
    ಶೂನ್ಯ visit(AstCMethodCall* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        ensureNullChecked(nodep->fromp());
        nodep->user1(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        nodep->user1(ಸತ್ಯ);
    }

    // NOPs
    ಶೂನ್ಯ visit(AstVar*) override {}

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit CastVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CastVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Cast class functions

ಶೂನ್ಯ V3Cast::castAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CastVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("cast", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
