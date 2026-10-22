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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Premit.h"

#ضمن "V3ConstPool.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr صحيح STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

صنف PremitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ثابت VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    AstNode* m_stmtp = nullptr;  // Current statement
    منطقي m_assignLhs = ܕܓܠܐ;  // Inside assignment lhs, don't breakup extracts
    منطقي m_inCons = ܕܓܠܐ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    منطقي needsTemp(AstNodeExpr* nodep) {
        // Consider adding a temp for this expression.
        ܐܢ (!m_stmtp) ܦܢܐ ܕܓܠܐ;  // Not under a statement
        ܐܢ (nodep->user1SetOnce()) ܦܢܐ ܕܓܠܐ;  // Already processed
        ܐܢ (!nodep->isWide()) ܦܢܐ ܕܓܠܐ;  // Not wide
        ܐܢ (m_assignLhs) ܦܢܐ ܕܓܠܐ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        ܦܢܐ ܫܪܪܐ;
    }

    فراغ checkNode(AstNodeExpr* nodep) {
        ܐܢ (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ساكن منطقي useConstPool(ثابت AstNodeExpr* nodep) {
        uint64_t width = 0;
        منطقي hasData = ܕܓܠܐ;
        منطقي hasDynamic = ܕܓܠܐ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ثابت منطقي emittable = nodep->forall([&](ثابت AstNodeExpr* exprp) {
            ܐܢ (VN_IS(exprp, NodeCons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ܦܢܐ ܫܪܪܐ;
            }
            ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
            ܐܢ (!constp) ܦܢܐ ܕܓܠܐ;
            width += static_cast<uint64_t>(constp->widthWords()) * VL_EDATASIZE;
            hasData = ܫܪܪܐ;
            ܦܢܐ !constp->num().isFourState();
        });
        ܐܢ (!emittable) ܦܢܐ ܕܓܠܐ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ܐܢ (hasDynamic) ܦܢܐ hasData;
        ܦܢܐ width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(AstNodeExpr* nodep) {
        // If enclosing statement is an assign
        AstNodeAssign* ثابت assignp = VN_CAST(m_stmtp, NodeAssign);
        ܐܢ (!assignp) ܦܢܐ nullptr;
        // of which 'nodep' is the RHS of
        ܐܢ (assignp->rhsp() != nodep) ܦܢܐ nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ثابت lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ܐܢ (!lRefp) ܦܢܐ nullptr;
        AstVar* ثابت varp = lRefp->varp();
        // And the RHS does not use the same variable
        ܐܢ (nodep->exists([varp](ثابت AstVarRef* refp) { ܦܢܐ refp->varp() == varp; })) {
            ܦܢܐ nullptr;
        }
        // Then return the LHS reference
        ܦܢܐ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(AstNodeExpr* nodep) {
        FileLine* ثابت flp = nodep->fileline();
        ثابت std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(ܫܪܪܐ);
        varp->noReset(ܫܪܪܐ);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        ܦܢܐ varp;
    }

    AstVar* createTemp(AstNodeExpr* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        FileLine* ثابت flp = nodep->fileline();
        AstVar* varp = nullptr;

        ܐܢ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ثابت refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ܐܚܪܢܐ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* ثابت assignp = جديد AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(جديد AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ܦܢܐ varp;
    }

    فراغ replaceWithOvrShift(AstNodeBiop* nodep) {
        FileLine* ثابت flp = nodep->fileline();
        AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* newp = nullptr;
        ܐܢ (VN_IS(nodep, ShiftL)) {
            newp = جديد AstShiftLOvr{flp, lhsp, rhsp};
        } ܐܚܪܢܐ ܐܢ (VN_IS(nodep, ShiftR)) {
            newp = جديد AstShiftROvr{flp, lhsp, rhsp};
        } ܐܚܪܢܐ {
            newp = جديد AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    فراغ visitShift(AstNodeBiop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        FileLine* ثابت flp = nodep->fileline();
        ܐܢ (ثابت AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ثابت منطقي isOversized
                = shiftp->num().mostSetBitP1() > 32
                  || (shiftp->num().toUInt() >= static_cast<uint32_t>(nodep->width()));
            ܐܢ (isOversized) {
                // If signed shift, replace with replicated MSB
                ܐܢ (VN_IS(nodep, ShiftRS)) {
                    AstNodeExpr* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
                    AstNodeExpr* ثابت msbp = جديد AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(جديد AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ܦܢܐ;
                }
                // Unsigned. If pure, replace with zero
                ܐܢ (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        جديد AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ܦܢܐ;
                }
                // Impure. Keep shift
                ܐܢ (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    ܦܢܐ;
                }
            }
        } ܐܚܪܢܐ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ܐܢ (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                ܦܢܐ;
            }
        }
        iterateChildren(nodep);
        checkNode(nodep);
    }

    ساكن منطقي rhsReadsLhs(ثابت AstNodeAssign* nodep) {
        ثابت VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ثابت AstVarRef* refp) {
            ܐܢ (refp->access().isWriteOrRW()) refp->varp()->user3(ܫܪܪܐ);
        });
        ܦܢܐ nodep->rhsp()->exists([](ثابت AstVarRef* refp) {
            ܦܢܐ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    ܐܢ (!m_cfuncp) ܦܢܐ; \
    ܐܢ ((stmtp)->user1SetOnce()) ܦܢܐ; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = ܕܓܠܐ; \
    m_stmtp = (stmtp);

    فراغ visit(AstNodeAssign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        ܐܢ (AstCvtArrayToPacked* ثابت packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            AstNodeExpr* ثابت exprLhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(exprLhsp);
            VL_DO_DANGLING(pushDeletep(packedp), packedp);
            AstNodeExpr* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(جديد AstCvtPackedToArray{rhsp->fileline(), rhsp, exprLhsp->dtypep()});
            nodep->dtypeFrom(exprLhsp);
        }
        // Direct assignment to a simple variable
        ܐܢ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            AstNode* ثابت rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ܐܢ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ܦܢܐ;
            ܐܢ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ܐܚܪܢܐ ܐܢ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(ܫܪܪܐ);
            } ܐܚܪܢܐ ܐܢ (rhsp->widthWords() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ܦܢܐ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ثابت منطقي isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ܐܢ (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } ܐܚܪܢܐ {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = ܫܪܪܐ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ܐܢ (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ܐܢ (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ܐܢ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    فراغ visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ܐܢ (v3Global.opt.autoflush()) {
            ثابت AstNode* searchp = nodep->nextp();
            ܟܡܐ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ܐܢ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->sameGateTree(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ܐܚܪܢܐ {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    جديد AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->cloneTreePure(ܫܪܪܐ) : nullptr});
            }
        }
    }
    فراغ visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ܐܢ (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(ܫܪܪܐ);
        } ܐܚܪܢܐ {
            createTemp(nodep->memp())->noSubst(ܫܪܪܐ);
        }
    }
    فراغ visit(AstNodeStmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    فراغ visit(AstShiftL* nodep) override { visitShift(nodep); }
    فراغ visit(AstShiftR* nodep) override { visitShift(nodep); }
    فراغ visit(AstShiftRS* nodep) override { visitShift(nodep); }

    فراغ visit(AstConst* nodep) override { checkNode(nodep); }
    // Operators
    فراغ visit(AstNodeTermop* nodep) override { checkNode(nodep); }
    فراغ visit(AstNodeUniop* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstNodeBiop* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstRand* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
        ܐܢ (!VN_IS(nodep->backp(), NodeAssign)) createTemp(nodep);
    }
    فراغ visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    فراغ visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ܕܓܠܐ;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        checkNode(nodep);
    }
    فراغ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ܕܓܠܐ;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    فراغ visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ܕܓܠܐ;
            iterateAndNextNull(nodep->bitp());
        }
        checkNode(nodep);
    }
    فراغ visit(AstNodeCons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ܐܢ (!m_inCons && nodep->isConstantTree()) {
            ܐܢ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            ܦܢܐ;
        }
        VL_RESTORER(m_inCons);
        m_inCons = ܫܪܪܐ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    فراغ visit(AstConsAssocItem* nodep) override {
        ܐܢ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    فراغ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ܐܢ (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ثابت varp = createTemp(nodep);
            varp->noSubst(ܫܪܪܐ);  // Do not re-inline in V3Subst
        }
    }
    فراغ visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ܐܢ (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ثابت lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(ܫܪܪܐ);

            FileLine* ثابت flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ثابت thenRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            ܐܢ (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            AstNodeExpr* ثابت thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ثابت thenAsspp = جديد AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ثابت elseRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            ܐܢ (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            AstNodeExpr* ثابت elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ثابت elseAsspp = جديد AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            AstNodeExpr* ثابت condp = nodep->condp()->unlinkFrBack();
            AstIf* ثابت ifp = جديد AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clearCacheTree();
            // Iterate the resulting assignments
            iterate(ifp);
            ܦܢܐ;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        AstNodeExpr* exprsp = nodep->exprsp();
        ܐܢ (nodep->exprFormat()) {
            exprsp = VN_AS(exprsp->nextp(), NodeExpr);
            لكل (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
                ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                ܐܢ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ثابت varp = createTemp(nodep->exprsp());
                    varp->noSubst(ܫܪܪܐ);
                    اكسر;
                }
            }
        }
        لكل (AstNodeExpr *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), NodeExpr);

            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            AstNodeExpr* ثابت subargp = fargp ? fargp->exprp() : argp;
            ܐܢ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ثابت valueVarp = createTemp(subargp);
                valueVarp->noSubst(ܫܪܪܐ);
                ܐܢ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ثابت nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(ܫܪܪܐ);
                }
                استمر;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ܐܢ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ثابت varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(ܫܪܪܐ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    فراغ visit(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
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

فراغ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("premit", 0, dumpTreeEitherLevel() >= 3);
}
