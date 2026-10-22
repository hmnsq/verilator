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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Premit.h"

#شامل "V3ConstPool.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr صحیح STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

رده PremitVisitor final : عمومی VNVisitor {
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
    Ast_گره* m_stmtp = nullptr;  // Current statement
    منطقی m_assignLhs = نادرست;  // Inside assignment lhs, don't breakup extracts
    منطقی m_inCons = نادرست;  // Inside a NodeCons, so not the root of a value

    // METHODS
    منطقی needsTemp(Ast_گره_عبارت* nodep) {
        // Consider adding a temp for this expression.
        اگر (!m_stmtp) بازگشت نادرست;  // Not under a statement
        اگر (nodep->user1SetOnce()) بازگشت نادرست;  // Already processed
        اگر (!nodep->isWide()) بازگشت نادرست;  // Not wide
        اگر (m_assignLhs) بازگشت نادرست;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        بازگشت درست;
    }

    تهی check_گره(Ast_گره_عبارت* nodep) {
        اگر (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ایستا منطقی useConstPool(ثابت Ast_گره_عبارت* nodep) {
        uint64_t width = 0;
        منطقی hasData = نادرست;
        منطقی hasDynamic = نادرست;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ثابت منطقی emittable = nodep->forall([&](ثابت Ast_گره_عبارت* exprp) {
            اگر (VN_IS(exprp, گره_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                بازگشت درست;
            }
            ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
            اگر (!constp) بازگشت نادرست;
            width += static_cast<uint64_t>(constp->width_واژگان()) * VL_EDATASIZE;
            hasData = درست;
            بازگشت !constp->رقم().isFourState();
        });
        اگر (!emittable) بازگشت نادرست;
        // Dynamically sized values allocate when built with data, so use the pool for them
        اگر (hasDynamic) بازگشت hasData;
        بازگشت width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_گره_عبارت* nodep) {
        // If enclosing statement is an assign
        Ast_گره_Assign* ثابت assignp = VN_CAST(m_stmtp, گره_Assign);
        اگر (!assignp) بازگشت nullptr;
        // of which 'nodep' is the RHS of
        اگر (assignp->rhsp() != nodep) بازگشت nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ثابت lRefp = VN_CAST(assignp->lhsp(), VarRef);
        اگر (!lRefp) بازگشت nullptr;
        AstVar* ثابت varp = lRefp->varp();
        // And the RHS does not use the same variable
        اگر (nodep->exists([varp](ثابت AstVarRef* refp) { بازگشت refp->varp() == varp; })) {
            بازگشت nullptr;
        }
        // Then return the LHS reference
        بازگشت lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_گره_عبارت* nodep) {
        File_سطر* ثابت flp = nodep->fileline();
        ثابت std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ثابت varp = جدید AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(درست);
        varp->noReset(درست);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        بازگشت varp;
    }

    AstVar* createTemp(Ast_گره_عبارت* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_سطر* ثابت flp = nodep->fileline();
        AstVar* varp = nullptr;

        اگر (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ثابت refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } وگرنه {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ثابت refp = جدید AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* ثابت assignp = جدید AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(جدید AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        بازگشت varp;
    }

    تهی replaceWithOvrShift(Ast_گره_Biop* nodep) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* newp = nullptr;
        اگر (VN_IS(nodep, ShiftL)) {
            newp = جدید AstShiftLOvr{flp, lhsp, rhsp};
        } وگرنه اگر (VN_IS(nodep, ShiftR)) {
            newp = جدید AstShiftROvr{flp, lhsp, rhsp};
        } وگرنه {
            newp = جدید AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visitShift(Ast_گره_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_سطر* ثابت flp = nodep->fileline();
        اگر (ثابت AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ثابت منطقی isOversized
                = shiftp->رقم().mostSetBitP1() > 32
                  || (shiftp->رقم().toUInt() >= static_cast<uint32_t>(nodep->width()));
            اگر (isOversized) {
                // If signed shift, replace with replicated MSB
                اگر (VN_IS(nodep, ShiftRS)) {
                    Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_گره_عبارت* ثابت msbp = جدید AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(جدید AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
                // Unsigned. If pure, replace with zero
                اگر (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        جدید AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
                // Impure. Keep shift
                اگر (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    بازگشت;
                }
            }
        } وگرنه {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            اگر (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                بازگشت;
            }
        }
        iterateChildren(nodep);
        check_گره(nodep);
    }

    ایستا منطقی rhsReadsLhs(ثابت Ast_گره_Assign* nodep) {
        ثابت VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ثابت AstVarRef* refp) {
            اگر (refp->access().isWriteOrRW()) refp->varp()->user3(درست);
        });
        بازگشت nodep->rhsp()->exists([](ثابت AstVarRef* refp) {
            بازگشت refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    تهی visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    اگر (!m_cfuncp) بازگشت; \
    اگر ((stmtp)->user1SetOnce()) بازگشت; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = نادرست; \
    m_stmtp = (stmtp);

    تهی visit(Ast_گره_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        اگر (AstCvtArrayToPacked* ثابت packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_گره_عبارت* ثابت عبارت_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(عبارت_Lhsp);
            VL_DO_DANGLING(افزودن_Deletep(packedp), packedp);
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(جدید AstCvtPackedToArray{rhsp->fileline(), rhsp, عبارت_Lhsp->dtypep()});
            nodep->dtypeFrom(عبارت_Lhsp);
        }
        // Direct assignment to a simple variable
        اگر (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_گره* ثابت rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            اگر (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) بازگشت;
            اگر (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } وگرنه اگر (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(درست);
            } وگرنه اگر (rhsp->width_واژگان() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                بازگشت;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ثابت منطقی isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        اگر (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } وگرنه {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = درست;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        اگر (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                اگر (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    تهی visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        اگر (v3Global.opt.autoflush()) {
            ثابت Ast_گره* searchp = nodep->nextp();
            وقتی (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            اگر (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_درخت(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } وگرنه {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    جدید AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_درخت_Pure(درست) : nullptr});
            }
        }
    }
    تهی visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(درست);
        } وگرنه {
            createTemp(nodep->memp())->noSubst(درست);
        }
    }
    تهی visit(Ast_گره_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    تهی visit(AstShiftL* nodep) override { visitShift(nodep); }
    تهی visit(AstShiftR* nodep) override { visitShift(nodep); }
    تهی visit(AstShiftRS* nodep) override { visitShift(nodep); }

    تهی visit(AstConst* nodep) override { check_گره(nodep); }
    // Operators
    تهی visit(Ast_گره_Termop* nodep) override { check_گره(nodep); }
    تهی visit(Ast_گره_Uniop* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(Ast_گره_Biop* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
        اگر (!VN_IS(nodep->backp(), گره_Assign)) createTemp(nodep);
    }
    تهی visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_گره(nodep);
    }
    تهی visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = نادرست;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_گره(nodep);
    }
    تهی visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = نادرست;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    تهی visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = نادرست;
            iterateAndNextNull(nodep->bitp());
        }
        check_گره(nodep);
    }
    تهی visit(Ast_گره_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        اگر (!m_inCons && nodep->is_Constant_درخت()) {
            اگر (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            بازگشت;
        }
        VL_RESTORER(m_inCons);
        m_inCons = درست;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    تهی visit(AstConsAssocItem* nodep) override {
        اگر (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    تهی visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ثابت varp = createTemp(nodep);
            varp->noSubst(درست);  // Do not re-inline in V3Subst
        }
    }
    تهی visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        اگر (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ثابت lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(درست);

            File_سطر* ثابت flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ثابت thenRefp = جدید AstVarRef{flp, varp, VAccess::WRITE};
            اگر (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_گره_عبارت* ثابت thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ثابت thenAsspp = جدید AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ثابت elseRefp = جدید AstVarRef{flp, varp, VAccess::WRITE};
            اگر (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_گره_عبارت* ثابت elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ثابت elseAsspp = جدید AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_گره_عبارت* ثابت condp = nodep->condp()->unlinkFrBack();
            AstIf* ثابت ifp = جدید AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(جدید AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_درخت();
            // Iterate the resulting assignments
            iterate(ifp);
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_گره_عبارت* exprsp = nodep->exprsp();
        اگر (nodep->عبارت_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), گره_عبارت);
            برای (Ast_گره* argp = exprsp; argp; argp = argp->nextp()) {
                ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                اگر (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ثابت varp = createTemp(nodep->exprsp());
                    varp->noSubst(درست);
                    بشکن;
                }
            }
        }
        برای (Ast_گره_عبارت *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), گره_عبارت);

            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_گره_عبارت* ثابت subargp = fargp ? fargp->exprp() : argp;
            اگر (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ثابت مقدار_Varp = createTemp(subargp);
                مقدار_Varp->noSubst(درست);
                اگر (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ثابت nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(درست);
                }
                ادامه;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            اگر (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ثابت varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(درست);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    تهی visit(AstVar*) override {}  // Don't hit varrefs under vars
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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

تهی V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("premit", 0, dump_درخت_Either_Level() >= 3);
}
