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
    Ast_كلي* m_stmtp = nullptr;  // Current statement
    منطقي m_assignLhs = خطأ;  // Inside assignment lhs, don't breakup extracts
    منطقي m_inCons = خطأ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    منطقي needsTemp(Ast_كلي_فرچ* nodep) {
        // Consider adding a temp for this expression.
        إذا (!m_stmtp) أرجع خطأ;  // Not under a statement
        إذا (nodep->user1SetOnce()) أرجع خطأ;  // Already processed
        إذا (!nodep->isWide()) أرجع خطأ;  // Not wide
        إذا (m_assignLhs) أرجع خطأ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        أرجع true;
    }

    فراغ check_كلي(Ast_كلي_فرچ* nodep) {
        إذا (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ساكن منطقي useConstPool(ثابت Ast_كلي_فرچ* nodep) {
        uint64_t width = 0;
        منطقي hasData = خطأ;
        منطقي hasDynamic = خطأ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ثابت منطقي emittable = nodep->forall([&](ثابت Ast_كلي_فرچ* exprp) {
            إذا (VN_IS(exprp, كلي_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                أرجع true;
            }
            ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
            إذا (!constp) أرجع خطأ;
            width += static_cast<uint64_t>(constp->width_كلموم()) * VL_EDATASIZE;
            hasData = true;
            أرجع !constp->لمب().isFourState();
        });
        إذا (!emittable) أرجع خطأ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        إذا (hasDynamic) أرجع hasData;
        أرجع width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_كلي_فرچ* nodep) {
        // If enclosing statement is an assign
        Ast_كلي_Assign* ثابت assignp = VN_CAST(m_stmtp, كلي_Assign);
        إذا (!assignp) أرجع nullptr;
        // of which 'nodep' is the RHS of
        إذا (assignp->rhsp() != nodep) أرجع nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ثابت lRefp = VN_CAST(assignp->lhsp(), VarRef);
        إذا (!lRefp) أرجع nullptr;
        AstVar* ثابت varp = lRefp->varp();
        // And the RHS does not use the same variable
        إذا (nodep->exists([varp](ثابت AstVarRef* refp) { أرجع refp->varp() == varp; })) {
            أرجع nullptr;
        }
        // Then return the LHS reference
        أرجع lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_كلي_فرچ* nodep) {
        File_لي* ثابت flp = nodep->fileline();
        ثابت std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(true);
        varp->noReset(true);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        أرجع varp;
    }

    AstVar* createTemp(Ast_كلي_فرچ* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_لي* ثابت flp = nodep->fileline();
        AstVar* varp = nullptr;

        إذا (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ثابت refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_بشيا(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } وإلا {
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
        أرجع varp;
    }

    فراغ replaceWithOvrShift(Ast_كلي_Biop* nodep) {
        File_لي* ثابت flp = nodep->fileline();
        Ast_كلي_فرچ* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_كلي_فرچ* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_كلي_فرچ* newp = nullptr;
        إذا (VN_IS(nodep, ShiftL)) {
            newp = جديد AstShiftLOvr{flp, lhsp, rhsp};
        } وإلا إذا (VN_IS(nodep, ShiftR)) {
            newp = جديد AstShiftROvr{flp, lhsp, rhsp};
        } وإلا {
            newp = جديد AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(تور_Deletep(nodep), nodep);
    }

    فراغ visitShift(Ast_كلي_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_لي* ثابت flp = nodep->fileline();
        إذا (ثابت AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ثابت منطقي isOversized
                = shiftp->لمب().mostSetBitP1() > 32
                  || (shiftp->لمب().toUInt() >= static_cast<uint32_t>(nodep->width()));
            إذا (isOversized) {
                // If signed shift, replace with replicated MSB
                إذا (VN_IS(nodep, ShiftRS)) {
                    Ast_كلي_فرچ* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_كلي_فرچ* ثابت msbp = جديد AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(جديد AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(تور_Deletep(nodep), nodep);
                    أرجع;
                }
                // Unsigned. If pure, replace with zero
                إذا (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        جديد AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(تور_Deletep(nodep), nodep);
                    أرجع;
                }
                // Impure. Keep shift
                إذا (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    أرجع;
                }
            }
        } وإلا {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            إذا (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                أرجع;
            }
        }
        iterateChildren(nodep);
        check_كلي(nodep);
    }

    ساكن منطقي rhsReadsLhs(ثابت Ast_كلي_Assign* nodep) {
        ثابت VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) refp->varp()->user3(true);
        });
        أرجع nodep->rhsp()->exists([](ثابت AstVarRef* refp) {
            أرجع refp->access().isReadOnly() && refp->varp()->user3();
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
    إذا (!m_cfuncp) أرجع; \
    إذا ((stmtp)->user1SetOnce()) أرجع; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = خطأ; \
    m_stmtp = (stmtp);

    فراغ visit(Ast_كلي_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        إذا (AstCvtArrayToPacked* ثابت packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_كلي_فرچ* ثابت فرچ_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(فرچ_Lhsp);
            VL_DO_DANGLING(تور_Deletep(packedp), packedp);
            Ast_كلي_فرچ* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(جديد AstCvtPackedToArray{rhsp->fileline(), rhsp, فرچ_Lhsp->dtypep()});
            nodep->dtypeFrom(فرچ_Lhsp);
        }
        // Direct assignment to a simple variable
        إذا (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_كلي* ثابت rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            إذا (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) أرجع;
            إذا (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } وإلا إذا (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(true);
            } وإلا إذا (rhsp->width_كلموم() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                أرجع;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ثابت منطقي isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        إذا (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } وإلا {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = true;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                إذا (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(تور_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    فراغ visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        إذا (v3Global.opt.autoflush()) {
            ثابت Ast_كلي* searchp = nodep->nextp();
            طالما (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            إذا (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_بشيا(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } وإلا {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    جديد AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_بشيا_Pure(true) : nullptr});
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
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(true);
        } وإلا {
            createTemp(nodep->memp())->noSubst(true);
        }
    }
    فراغ visit(Ast_كلي_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    فراغ visit(AstShiftL* nodep) override { visitShift(nodep); }
    فراغ visit(AstShiftR* nodep) override { visitShift(nodep); }
    فراغ visit(AstShiftRS* nodep) override { visitShift(nodep); }

    فراغ visit(AstConst* nodep) override { check_كلي(nodep); }
    // Operators
    فراغ visit(Ast_كلي_Termop* nodep) override { check_كلي(nodep); }
    فراغ visit(Ast_كلي_Uniop* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(Ast_كلي_Biop* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
        إذا (!VN_IS(nodep->backp(), كلي_Assign)) createTemp(nodep);
    }
    فراغ visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_كلي(nodep);
    }
    فراغ visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = خطأ;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_كلي(nodep);
    }
    فراغ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = خطأ;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    فراغ visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = خطأ;
            iterateAndNextNull(nodep->bitp());
        }
        check_كلي(nodep);
    }
    فراغ visit(Ast_كلي_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        إذا (!m_inCons && nodep->is_Constant_بشيا()) {
            إذا (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(تور_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            أرجع;
        }
        VL_RESTORER(m_inCons);
        m_inCons = true;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    فراغ visit(AstConsAssocItem* nodep) override {
        إذا (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    فراغ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ثابت varp = createTemp(nodep);
            varp->noSubst(true);  // Do not re-inline in V3Subst
        }
    }
    فراغ visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        إذا (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ثابت lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(true);

            File_لي* ثابت flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ثابت thenRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            إذا (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_كلي_فرچ* ثابت thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ثابت thenAsspp = جديد AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ثابت elseRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            إذا (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_كلي_فرچ* ثابت elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ثابت elseAsspp = جديد AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_كلي_فرچ* ثابت condp = nodep->condp()->unlinkFrBack();
            AstIf* ثابت ifp = جديد AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(تور_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_بشيا();
            // Iterate the resulting assignments
            iterate(ifp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_كلي_فرچ* exprsp = nodep->exprsp();
        إذا (nodep->فرچ_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), كلي_فرچ);
            لكل (Ast_كلي* argp = exprsp; argp; argp = argp->nextp()) {
                ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                إذا (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ثابت varp = createTemp(nodep->exprsp());
                    varp->noSubst(true);
                    اكسر;
                }
            }
        }
        لكل (Ast_كلي_فرچ *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), كلي_فرچ);

            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_كلي_فرچ* ثابت subargp = fargp ? fargp->exprp() : argp;
            إذا (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ثابت درج_Varp = createTemp(subargp);
                درج_Varp->noSubst(true);
                إذا (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ثابت nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(true);
                }
                استمر;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            إذا (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ثابت varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(true);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    فراغ visit(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ visit(Ast_كلي* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_بشيا("premit", 0, dump_بشيا_Either_Level() >= 3);
}
