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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Premit.h"

#অন্তর্ভাৱ "V3ConstPool.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr পূর্ণাঙ্ক STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

শ্ৰেণী PremitVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ধ্রুৱ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_গ্ৰন্থি* m_stmtp = nullptr;  // Current statement
    তার্কিক m_assignLhs = মিছা;  // Inside assignment lhs, don't breakup extracts
    তার্কিক m_inCons = মিছা;  // Inside a NodeCons, so not the root of a value

    // METHODS
    তার্কিক needsTemp(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // Consider adding a temp for this expression.
        যদি (!m_stmtp) ঘূৰাই মিছা;  // Not under a statement
        যদি (nodep->user1SetOnce()) ঘূৰাই মিছা;  // Already processed
        যদি (!nodep->isWide()) ঘূৰাই মিছা;  // Not wide
        যদি (m_assignLhs) ঘূৰাই মিছা;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        ঘূৰাই সঁচা;
    }

    শূন্য check_গ্ৰন্থি(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    স্থির তার্কিক useConstPool(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        uint64_t width = 0;
        তার্কিক hasData = মিছা;
        তার্কিক hasDynamic = মিছা;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ধ্রুৱ তার্কিক emittable = nodep->forall([&](ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
            যদি (VN_IS(exprp, গ্ৰন্থি_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ঘূৰাই সঁচা;
            }
            ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(exprp, Const);
            যদি (!constp) ঘূৰাই মিছা;
            width += static_cast<uint64_t>(constp->width_শব্দগুলি()) * VL_EDATASIZE;
            hasData = সঁচা;
            ঘূৰাই !constp->অঙ্ক().isFourState();
        });
        যদি (!emittable) ঘূৰাই মিছা;
        // Dynamically sized values allocate when built with data, so use the pool for them
        যদি (hasDynamic) ঘূৰাই hasData;
        ঘূৰাই width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // If enclosing statement is an assign
        Ast_গ্ৰন্থি_Assign* ধ্রুৱ assignp = VN_CAST(m_stmtp, গ্ৰন্থি_Assign);
        যদি (!assignp) ঘূৰাই nullptr;
        // of which 'nodep' is the RHS of
        যদি (assignp->rhsp() != nodep) ঘূৰাই nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ধ্রুৱ lRefp = VN_CAST(assignp->lhsp(), VarRef);
        যদি (!lRefp) ঘূৰাই nullptr;
        AstVar* ধ্রুৱ varp = lRefp->varp();
        // And the RHS does not use the same variable
        যদি (nodep->exists([varp](ধ্রুৱ AstVarRef* refp) { ঘূৰাই refp->varp() == varp; })) {
            ঘূৰাই nullptr;
        }
        // Then return the LHS reference
        ঘূৰাই lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        ধ্রুৱ std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ধ্রুৱ varp = নূতন AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(সঁচা);
        varp->noReset(সঁচা);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        ঘূৰাই varp;
    }

    AstVar* createTemp(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        AstVar* varp = nullptr;

        যদি (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ধ্রুৱ refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } অন্যথা {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* ধ্রুৱ assignp = নূতন AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(নূতন AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ঘূৰাই varp;
    }

    শূন্য replaceWithOvrShift(Ast_গ্ৰন্থি_Biop* nodep) {
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
        যদি (VN_IS(nodep, ShiftL)) {
            newp = নূতন AstShiftLOvr{flp, lhsp, rhsp};
        } অন্যথা যদি (VN_IS(nodep, ShiftR)) {
            newp = নূতন AstShiftROvr{flp, lhsp, rhsp};
        } অন্যথা {
            newp = নূতন AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visitShift(Ast_গ্ৰন্থি_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        যদি (ধ্রুৱ AstConst* ধ্রুৱ shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ধ্রুৱ তার্কিক isOversized
                = shiftp->অঙ্ক().mostSetBitP1() > 32
                  || (shiftp->অঙ্ক().toUInt() >= static_cast<uint32_t>(nodep->width()));
            যদি (isOversized) {
                // If signed shift, replace with replicated MSB
                যদি (VN_IS(nodep, ShiftRS)) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ msbp = নূতন AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(নূতন AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
                // Unsigned. If pure, replace with zero
                যদি (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        নূতন AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
                // Impure. Keep shift
                যদি (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    ঘূৰাই;
                }
            }
        } অন্যথা {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            যদি (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                ঘূৰাই;
            }
        }
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }

    স্থির তার্কিক rhsReadsLhs(ধ্রুৱ Ast_গ্ৰন্থি_Assign* nodep) {
        ধ্রুৱ VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ধ্রুৱ AstVarRef* refp) {
            যদি (refp->access().isWriteOrRW()) refp->varp()->user3(সঁচা);
        });
        ঘূৰাই nodep->rhsp()->exists([](ধ্রুৱ AstVarRef* refp) {
            ঘূৰাই refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    শূন্য visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    যদি (!m_cfuncp) ঘূৰাই; \
    যদি ((stmtp)->user1SetOnce()) ঘূৰাই; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = মিছা; \
    m_stmtp = (stmtp);

    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        যদি (AstCvtArrayToPacked* ধ্রুৱ packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ ৰাশিমালা_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(ৰাশিমালা_Lhsp);
            VL_DO_DANGLING(ঠেলো_Deletep(packedp), packedp);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(নূতন AstCvtPackedToArray{rhsp->fileline(), rhsp, ৰাশিমালা_Lhsp->dtypep()});
            nodep->dtypeFrom(ৰাশিমালা_Lhsp);
        }
        // Direct assignment to a simple variable
        যদি (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_গ্ৰন্থি* ধ্রুৱ rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            যদি (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ঘূৰাই;
            যদি (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } অন্যথা যদি (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(সঁচা);
            } অন্যথা যদি (rhsp->width_শব্দগুলি() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ঘূৰাই;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ধ্রুৱ তার্কিক isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        যদি (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } অন্যথা {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = সঁচা;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        যদি (AstVarRef* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            যদি (AstVarRef* ধ্রুৱ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                যদি (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    শূন্য visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        যদি (v3Global.opt.autoflush()) {
            ধ্রুৱ Ast_গ্ৰন্থি* searchp = nodep->nextp();
            যেতিয়ালৈ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            যদি (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_বৃক্ষ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } অন্যথা {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    নূতন AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_বৃক্ষ_Pure(সঁচা) : nullptr});
            }
        }
    }
    শূন্য visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(সঁচা);
        } অন্যথা {
            createTemp(nodep->memp())->noSubst(সঁচা);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    শূন্য visit(AstShiftL* nodep) override { visitShift(nodep); }
    শূন্য visit(AstShiftR* nodep) override { visitShift(nodep); }
    শূন্য visit(AstShiftRS* nodep) override { visitShift(nodep); }

    শূন্য visit(AstConst* nodep) override { check_গ্ৰন্থি(nodep); }
    // Operators
    শূন্য visit(Ast_গ্ৰন্থি_Termop* nodep) override { check_গ্ৰন্থি(nodep); }
    শূন্য visit(Ast_গ্ৰন্থি_Uniop* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Biop* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
        যদি (!VN_IS(nodep->backp(), গ্ৰন্থি_Assign)) createTemp(nodep);
    }
    শূন্য visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = মিছা;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = মিছা;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    শূন্য visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = মিছা;
            iterateAndNextNull(nodep->bitp());
        }
        check_গ্ৰন্থি(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        যদি (!m_inCons && nodep->is_Constant_বৃক্ষ()) {
            যদি (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            ঘূৰাই;
        }
        VL_RESTORER(m_inCons);
        m_inCons = সঁচা;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    শূন্য visit(AstConsAssocItem* nodep) override {
        যদি (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    শূন্য visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        যদি (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ধ্রুৱ varp = createTemp(nodep);
            varp->noSubst(সঁচা);  // Do not re-inline in V3Subst
        }
    }
    শূন্য visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        যদি (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ধ্রুৱ lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(সঁচা);

            File_সাৰি* ধ্রুৱ flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ধ্রুৱ thenRefp = নূতন AstVarRef{flp, varp, VAccess::WRITE};
            যদি (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ধ্রুৱ thenAsspp = নূতন AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ধ্রুৱ elseRefp = নূতন AstVarRef{flp, varp, VAccess::WRITE};
            যদি (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ধ্রুৱ elseAsspp = নূতন AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = nodep->condp()->unlinkFrBack();
            AstIf* ধ্রুৱ ifp = নূতন AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_বৃক্ষ();
            // Iterate the resulting assignments
            iterate(ifp);
            ঘূৰাই;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_গ্ৰন্থি_ৰাশিমালা* exprsp = nodep->exprsp();
        যদি (nodep->ৰাশিমালা_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), গ্ৰন্থি_ৰাশিমালা);
            বাবে (Ast_গ্ৰন্থি* argp = exprsp; argp; argp = argp->nextp()) {
                ধ্রুৱ AstSFormatArg* ধ্রুৱ fargp = VN_CAST(argp, SFormatArg);
                যদি (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ধ্রুৱ varp = createTemp(nodep->exprsp());
                    varp->noSubst(সঁচা);
                    ভঙ্গ;
                }
            }
        }
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), গ্ৰন্থি_ৰাশিমালা);

            AstSFormatArg* ধ্রুৱ fargp = VN_CAST(argp, SFormatArg);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ subargp = fargp ? fargp->exprp() : argp;
            যদি (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ধ্রুৱ মান_Varp = createTemp(subargp);
                মান_Varp->noSubst(সঁচা);
                যদি (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ধ্রুৱ nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(সঁচা);
                }
                অনুৱর্ত;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            যদি (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ধ্রুৱ varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(সঁচা);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    শূন্য visit(AstVar*) override {}  // Don't hit varrefs under vars
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
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

শূন্য V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("premit", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
