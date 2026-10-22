// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for premit nodes
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Premit's Transformations:
//
// Each module:
//      For each wide OP, make a a temporary variable with the wide value
//      For each deep expression, assign expression to temporary.
//
// Each display (independent transformation; here as Premit is a good point)
//      If autoflush, insert a flush
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Premit.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ಪೂರ್ಣಾಙ್ಕ STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

ವರ್ಗ PremitVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ಗಂಟು* m_stmtp = nullptr;  // Current statement
    ತಾರ್ಕಿಕ m_assignLhs = ಅಸತ್ಯ;  // Inside assignment lhs, don't breakup extracts
    ತಾರ್ಕಿಕ m_inCons = ಅಸತ್ಯ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    ತಾರ್ಕಿಕ needsTemp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Consider adding a temp for this expression.
        ಯದಿ (!m_stmtp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Not under a statement
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Already processed
        ಯದಿ (!nodep->isWide()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Not wide
        ಯದಿ (m_assignLhs) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ check_ಗಂಟು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ಸ್ಥಿರ ತಾರ್ಕಿಕ useConstPool(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        uint64_t width = 0;
        ತಾರ್ಕಿಕ hasData = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ hasDynamic = ಅಸತ್ಯ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ಧ್ರುವ ತಾರ್ಕಿಕ emittable = nodep->forall([&](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
            ಯದಿ (VN_IS(exprp, ಗಂಟು_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(exprp, Const);
            ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            width += static_cast<uint64_t>(constp->width_ಪದಗಳು()) * VL_EDATASIZE;
            hasData = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ !constp->ಅಂಕೆ().isFourState();
        });
        ಯದಿ (!emittable) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ಯದಿ (hasDynamic) ಪ್ರತ್ಯಾಗಮ hasData;
        ಪ್ರತ್ಯಾಗಮ width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // If enclosing statement is an assign
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_CAST(m_stmtp, ಗಂಟು_Assign);
        ಯದಿ (!assignp) ಪ್ರತ್ಯಾಗಮ nullptr;
        // of which 'nodep' is the RHS of
        ಯದಿ (assignp->rhsp() != nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ಧ್ರುವ lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ಯದಿ (!lRefp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVar* ಧ್ರುವ varp = lRefp->varp();
        // And the RHS does not use the same variable
        ಯದಿ (nodep->exists([varp](ಧ್ರುವ AstVarRef* refp) { ಪ್ರತ್ಯಾಗಮ refp->varp() == varp; })) {
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // Then return the LHS reference
        ಪ್ರತ್ಯಾಗಮ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಧ್ರುವ std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(ಸತ್ಯ);
        varp->noReset(ಸತ್ಯ);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    AstVar* createTemp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstVar* varp = nullptr;

        ಯದಿ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ಧ್ರುವ refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ಅನ್ಯಥಾ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(ನೂತನ AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    ಶೂನ್ಯ replaceWithOvrShift(Ast_ಗಂಟು_Biop* nodep) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
        ಯದಿ (VN_IS(nodep, ShiftL)) {
            newp = ನೂತನ AstShiftLOvr{flp, lhsp, rhsp};
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, ShiftR)) {
            newp = ನೂತನ AstShiftROvr{flp, lhsp, rhsp};
        } ಅನ್ಯಥಾ {
            newp = ನೂತನ AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visitShift(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ಧ್ರುವ ತಾರ್ಕಿಕ isOversized
                = shiftp->ಅಂಕೆ().mostSetBitP1() > 32
                  || (shiftp->ಅಂಕೆ().toUInt() >= static_cast<uint32_t>(nodep->width()));
            ಯದಿ (isOversized) {
                // If signed shift, replace with replicated MSB
                ಯದಿ (VN_IS(nodep, ShiftRS)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ msbp = ನೂತನ AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(ನೂತನ AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Unsigned. If pure, replace with zero
                ಯದಿ (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        ನೂತನ AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Impure. Keep shift
                ಯದಿ (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        } ಅನ್ಯಥಾ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ಯದಿ (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ rhsReadsLhs(ಧ್ರುವ Ast_ಗಂಟು_Assign* nodep) {
        ಧ್ರುವ VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ಧ್ರುವ AstVarRef* refp) {
            ಯದಿ (refp->access().isWriteOrRW()) refp->varp()->user3(ಸತ್ಯ);
        });
        ಪ್ರತ್ಯಾಗಮ nodep->rhsp()->exists([](ಧ್ರುವ AstVarRef* refp) {
            ಪ್ರತ್ಯಾಗಮ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    ಯದಿ (!m_cfuncp) ಪ್ರತ್ಯಾಗಮ; \
    ಯದಿ ((stmtp)->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = ಅಸತ್ಯ; \
    m_stmtp = (stmtp);

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        ಯದಿ (AstCvtArrayToPacked* ಧ್ರುವ packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(ಅಭಿವ್ಯಕ್ತಿ_Lhsp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(packedp), packedp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(ನೂತನ AstCvtPackedToArray{rhsp->fileline(), rhsp, ಅಭಿವ್ಯಕ್ತಿ_Lhsp->dtypep()});
            nodep->dtypeFrom(ಅಭಿವ್ಯಕ್ತಿ_Lhsp);
        }
        // Direct assignment to a simple variable
        ಯದಿ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ಗಂಟು* ಧ್ರುವ rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ಯದಿ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (rhsp->width_ಪದಗಳು() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ಧ್ರುವ ತಾರ್ಕಿಕ isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ಯದಿ (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } ಅನ್ಯಥಾ {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = ಸತ್ಯ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ಯದಿ (AstVarRef* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ಯದಿ (AstVarRef* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ಯದಿ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ಯದಿ (v3Global.opt.autoflush()) {
            ಧ್ರುವ Ast_ಗಂಟು* searchp = nodep->nextp();
            ಯಾವತ್ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ಯದಿ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_ವೃಕ್ಷ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ಅನ್ಯಥಾ {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    ನೂತನ AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ) : nullptr});
            }
        }
    }
    ಶೂನ್ಯ visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            createTemp(nodep->memp())->noSubst(ಸತ್ಯ);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    ಶೂನ್ಯ visit(AstShiftL* nodep) override { visitShift(nodep); }
    ಶೂನ್ಯ visit(AstShiftR* nodep) override { visitShift(nodep); }
    ಶೂನ್ಯ visit(AstShiftRS* nodep) override { visitShift(nodep); }

    ಶೂನ್ಯ visit(AstConst* nodep) override { check_ಗಂಟು(nodep); }
    // Operators
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Termop* nodep) override { check_ಗಂಟು(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
        ಯದಿ (!VN_IS(nodep->backp(), ಗಂಟು_Assign)) createTemp(nodep);
    }
    ಶೂನ್ಯ visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ಅಸತ್ಯ;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ಅಸತ್ಯ;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    ಶೂನ್ಯ visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ಅಸತ್ಯ;
            iterateAndNextNull(nodep->bitp());
        }
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ಯದಿ (!m_inCons && nodep->is_Constant_ವೃಕ್ಷ()) {
            ಯದಿ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        VL_RESTORER(m_inCons);
        m_inCons = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    ಶೂನ್ಯ visit(AstConsAssocItem* nodep) override {
        ಯದಿ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    ಶೂನ್ಯ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ಯದಿ (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ಧ್ರುವ varp = createTemp(nodep);
            varp->noSubst(ಸತ್ಯ);  // Do not re-inline in V3Subst
        }
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ಯದಿ (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ಧ್ರುವ lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(ಸತ್ಯ);

            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ಧ್ರುವ thenRefp = ನೂತನ AstVarRef{flp, varp, VAccess::WRITE};
            ಯದಿ (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ಧ್ರುವ thenAsspp = ನೂತನ AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ಧ್ರುವ elseRefp = ನೂತನ AstVarRef{flp, varp, VAccess::WRITE};
            ಯದಿ (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ಧ್ರುವ elseAsspp = ನೂತನ AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = nodep->condp()->unlinkFrBack();
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(ನೂತನ AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_ವೃಕ್ಷ();
            // Iterate the resulting assignments
            iterate(ifp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp = nodep->exprsp();
        ಯದಿ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಪ್ರತಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) {
                ಧ್ರುವ AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
                ಯದಿ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ಧ್ರುವ varp = createTemp(nodep->exprsp());
                    varp->noSubst(ಸತ್ಯ);
                    ಭಙ್ಗ;
                }
            }
        }
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);

            AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ subargp = fargp ? fargp->exprp() : argp;
            ಯದಿ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ಧ್ರುವ ಮೌಲ್ಯ_Varp = createTemp(subargp);
                ಮೌಲ್ಯ_Varp->noSubst(ಸತ್ಯ);
                ಯದಿ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ಧ್ರುವ nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(ಸತ್ಯ);
                }
                ಅನುವರ್ತ;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ಯದಿ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ಧ್ರುವ varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(ಸತ್ಯ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    ಶೂನ್ಯ visit(AstVar*) override {}  // Don't hit varrefs under vars
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::addStat("Optimizations, Prelim extracted value to ConstPool",
                         m_extractedToConstPool);
        V3Stats::addStat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

ಶೂನ್ಯ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("premit", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
