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

constexpr صحيح STATIC_CONST_MIN_ويڪر = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

صنف PremitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_منزل_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = خالي_اشارو;  // Current block
    سائيز_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ڳنڍ* m_stmtp = خالي_اشارو;  // Current statement
    منطقي m_سونپيو_Lhs = خطأ;  // Inside assignment lhs, don't breakup extracts
    منطقي m_اندريون_Cons = خطأ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    منطقي needs_عارضي(Ast_ڳنڍ_اظهار* nodep) {
        // Consider adding a temp for this expression.
        إذا (!m_stmtp) أرجع خطأ;  // Not under a statement
        إذا (nodep->user1_سيٽ_Once()) أرجع خطأ;  // Already processed
        إذا (!nodep->isWide()) أرجع خطأ;  // Not wide
        إذا (m_سونپيو_Lhs) أرجع خطأ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->پهريون_Abovep(), ArraySel), nodep, "Should have been ignored");
        أرجع سچ;
    }

    فراغ check_ڳنڍ(Ast_ڳنڍ_اظهار* nodep) {
        إذا (needs_عارضي(nodep)) create_عارضي(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ساكن منطقي useConstPool(ثابت Ast_ڳنڍ_اظهار* nodep) {
        uint64_t ويڪر = 0;
        منطقي has_معلومات = خطأ;
        منطقي hasDynamic = خطأ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ثابت منطقي emittable = nodep->forall([&](ثابت Ast_ڳنڍ_اظهار* exprp) {
            إذا (VN_IS(exprp, ڳنڍ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                أرجع سچ;
            }
            ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
            إذا (!constp) أرجع خطأ;
            ويڪر += static_cast<uint64_t>(constp->ويڪر_لفظ()) * VL_EDATASIZE;
            has_معلومات = سچ;
            أرجع !constp->انگ().isFourState();
        });
        إذا (!emittable) أرجع خطأ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        إذا (hasDynamic) أرجع has_معلومات;
        أرجع ويڪر >= static_cast<uint64_t>(STATIC_CONST_MIN_ويڪر);
    }

    AstVarRef* is_Rhs_جو_سونپيو_منزل_Var(Ast_ڳنڍ_اظهار* nodep) {
        // If enclosing statement is an assign
        Ast_ڳنڍ_سونپيو* ثابت assignp = VN_CAST(m_stmtp, ڳنڍ_سونپيو);
        إذا (!assignp) أرجع خالي_اشارو;
        // of which 'nodep' is the RHS of
        إذا (assignp->rhsp() != nodep) أرجع خالي_اشارو;
        // and the LHS is a sipmple variable reference
        AstVarRef* ثابت lRefp = VN_CAST(assignp->lhsp(), VarRef);
        إذا (!lRefp) أرجع خالي_اشارو;
        AstVar* ثابت varp = lRefp->varp();
        // And the RHS does not use the same variable
        إذا (nodep->exists([varp](ثابت AstVarRef* refp) { أرجع refp->varp() == varp; })) {
            أرجع خالي_اشارو;
        }
        // Then return the LHS reference
        أرجع lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ڳنڍ_اظهار* nodep) {
        File_سٽ* ثابت flp = nodep->fileline();
        ثابت std::string name = "__Vtemp_" + std::منزل_string(++m_tmpVarCnt);
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(سچ);
        varp->noReset(سچ);
        m_cfuncp->جوڙيو_Varsp(varp);
        ++m_temporaryVarsCreated;
        أرجع varp;
    }

    AstVar* create_عارضي(Ast_ڳنڍ_اظهار* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_پٺ(&relinker);

        File_سٽ* ثابت flp = nodep->fileline();
        AstVar* varp = خالي_اشارو;

        إذا (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ثابت refp = V3ConstPool::ڳوليو(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            ++m_extracted_منزل_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } وإلا {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->جوڙيو_هتي_This_As_ايندڙ(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(جديد AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        أرجع varp;
    }

    فراغ replace_With_Ovr_سرڪايو(Ast_ڳنڍ_Biop* nodep) {
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* newp = خالي_اشارو;
        إذا (VN_IS(nodep, سرڪايو_L)) {
            newp = جديد Ast_سرڪايو_LOvr{flp, lhsp, rhsp};
        } وإلا إذا (VN_IS(nodep, سرڪايو_R)) {
            newp = جديد Ast_سرڪايو_ROvr{flp, lhsp, rhsp};
        } وإلا {
            newp = جديد Ast_سرڪايو_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو_سرڪايو(Ast_ڳنڍ_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, سرڪايو_L) || VN_IS(nodep, سرڪايو_R) || VN_IS(nodep, سرڪايو_RS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (ثابت AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ثابت منطقي isOversized
                = shiftp->انگ().most_سيٽ_ٻٽو_انگ_P1() > 32
                  || (shiftp->انگ().منزل_UInt() >= static_cast<uint32_t>(nodep->ويڪر()));
            إذا (isOversized) {
                // If signed shift, replace with replicated MSB
                إذا (VN_IS(nodep, سرڪايو_RS)) {
                    Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ();
                    Ast_ڳنڍ_اظهار* ثابت msbp = جديد AstSel{flp, lhsp, lhsp->ويڪر_Min() - 1, 1};
                    nodep->replaceWithKeepDType(جديد AstExtendS{flp, msbp, nodep->ويڪر()});
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    أرجع;
                }
                // Unsigned. If pure, replace with zero
                إذا (nodep->lhsp()->is_خالص()) {
                    nodep->replaceWithKeepDType(
                        جديد AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    أرجع;
                }
                // Impure. Keep shift
                إذا (!nodep->isWide()) {
                    replace_With_Ovr_سرڪايو(nodep);
                    أرجع;
                }
            }
        } وإلا {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            إذا (!nodep->isWide() && (nodep->ويڪر() < (1LL << nodep->rhsp()->ويڪر_Min()))) {
                replace_With_Ovr_سرڪايو(nodep);
                أرجع;
            }
        }
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }

    ساكن منطقي rhsReadsLhs(ثابت Ast_ڳنڍ_سونپيو* nodep) {
        ثابت VNUser3_اندريون_Use user3_اندريون_Use;
        nodep->lhsp()->foreach([](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) refp->varp()->user3(سچ);
        });
        أرجع nodep->rhsp()->exists([](ثابت AstVarRef* refp) {
            أرجع refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    فراغ گهمو(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#وصف_ڪريو شروعات_STATEMENT_OR_RETURN(stmtp) \
    إذا (!m_cfuncp) أرجع; \
    إذا ((stmtp)->user1_سيٽ_Once()) أرجع; \
    VL_RESTORER(m_سونپيو_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_سونپيو_Lhs = خطأ; \
    m_stmtp = (stmtp);

    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        شروعات_STATEMENT_OR_RETURN(nodep);

        إذا (Ast_Cvt_Array_منزل_Packed* ثابت packedp = VN_CAST(nodep->lhsp(), Cvt_Array_منزل_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ڳنڍ_اظهار* ثابت اظهار_Lhsp = packedp->fromp()->unlink_Fr_پٺ();
            packedp->replaceWith(اظهار_Lhsp);
            VL_DO_DANGLING(ڌڪو_Deletep(packedp), packedp);
            Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
            nodep->rhsp(جديد Ast_Cvt_Packed_منزل_Array{rhsp->fileline(), rhsp, اظهار_Lhsp->dtypep()});
            nodep->dtypeFrom(اظهار_Lhsp);
        }
        // Direct assignment to a simple variable
        إذا (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ڳنڍ* ثابت rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            إذا (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) أرجع;
            إذا (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } وإلا إذا (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(سچ);
            } وإلا إذا (rhsp->ويڪر_لفظ() * VL_EDATASIZE < STATIC_CONST_MIN_ويڪر) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                أرجع;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ثابت منطقي isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        إذا (!isAtomic && rhsReadsLhs(nodep)) {
            create_عارضي(nodep->rhsp());
        } وإلا {
            iterate_And_ايندڙ_Null(nodep->rhsp());
        }

        m_سونپيو_Lhs = سچ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_ايندڙ_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                إذا (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                }
            }
        }
    }
    فراغ گهمو(AstDisplay* nodep) override {
        شروعات_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        إذا (v3Global.opt.autoflush()) {
            ثابت Ast_ڳنڍ* searchp = nodep->nextp();
            طالما (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            إذا (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_وڻ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } وإلا {
                UINFO(4, "Autoflush " << nodep);
                nodep->جوڙيو_ايندڙ_هتي(
                    جديد AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_وڻ_خالص(سچ) : خالي_اشارو});
            }
        }
    }
    فراغ گهمو(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        شروعات_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(سچ);
        } وإلا {
            create_عارضي(nodep->memp())->noSubst(سچ);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        شروعات_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#وصف_رد شروعات_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    فراغ گهمو(Ast_سرڪايو_L* nodep) override { گهمو_سرڪايو(nodep); }
    فراغ گهمو(Ast_سرڪايو_R* nodep) override { گهمو_سرڪايو(nodep); }
    فراغ گهمو(Ast_سرڪايو_RS* nodep) override { گهمو_سرڪايو(nodep); }

    فراغ گهمو(AstConst* nodep) override { check_ڳنڍ(nodep); }
    // Operators
    فراغ گهمو(Ast_ڳنڍ_Termop* nodep) override { check_ڳنڍ(nodep); }
    فراغ گهمو(Ast_ڳنڍ_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(Ast_Cvt_Array_منزل_Packed* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(Ast_Cvt_Packed_منزل_Array* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
        إذا (!VN_IS(nodep->backp(), ڳنڍ_سونپيو)) create_عارضي(nodep);
    }
    فراغ گهمو(Ast_Cvt_Unpack_منزل_قطار* nodep) override {
        iterateChildren(nodep);
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(AstSel* nodep) override {
        iterate_And_ايندڙ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_سونپيو_Lhs);
            m_سونپيو_Lhs = خطأ;
            iterate_And_ايندڙ_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(AstArraySel* nodep) override {
        iterate_And_ايندڙ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_سونپيو_Lhs);
            m_سونپيو_Lhs = خطأ;
            iterate_And_ايندڙ_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    فراغ گهمو(AstAssocSel* nodep) override {
        iterate_And_ايندڙ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_سونپيو_Lhs);
            m_سونپيو_Lhs = خطأ;
            iterate_And_ايندڙ_Null(nodep->bitp());
        }
        check_ڳنڍ(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        إذا (!m_اندريون_Cons && nodep->is_Constant_وڻ()) {
            إذا (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::ڳوليو(nodep));
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                ++m_extracted_منزل_Const_Pool;
            }
            أرجع;
        }
        VL_RESTORER(m_اندريون_Cons);
        m_اندريون_Cons = سچ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    فراغ گهمو(Ast_Cons_Assoc_شي* nodep) override {
        إذا (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    فراغ گهمو(Ast_ميلاپ_Masked* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1_سيٽ_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* ثابت varp = create_عارضي(nodep);
            varp->noSubst(سچ);  // Do not re-inline in V3Subst
        }
    }
    فراغ گهمو(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        إذا (needs_عارضي(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ثابت lRefp = is_Rhs_جو_سونپيو_منزل_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(سچ);

            File_سٽ* ثابت flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ثابت thenRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            إذا (lRefp) thenRefp->self_اشارو(lRefp->self_اشارو());
            Ast_ڳنڍ_اظهار* ثابت thenExprp = nodep->thenp()->unlink_Fr_پٺ();
            Ast_سونپيو* ثابت thenAsspp = جديد Ast_سونپيو{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ثابت elseRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            إذا (lRefp) elseRefp->self_اشارو(lRefp->self_اشارو());
            Ast_ڳنڍ_اظهار* ثابت elseExprp = nodep->elsep()->unlink_Fr_پٺ();
            Ast_سونپيو* ثابت elseAsspp = جديد Ast_سونپيو{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ڳنڍ_اظهار* ثابت condp = nodep->condp()->unlink_Fr_پٺ();
            AstIf* ثابت ifp = جديد AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->جوڙيو_هتي_This_As_ايندڙ(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_وڻ();
            // Iterate the resulting assignments
            iterate(ifp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ڳنڍ_اظهار* exprsp = nodep->exprsp();
        إذا (nodep->اظهار_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ڳنڍ_اظهار);
            لكل (Ast_ڳنڍ* argp = exprsp; argp; argp = argp->nextp()) {
                ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                إذا (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ثابت varp = create_عارضي(nodep->exprsp());
                    varp->noSubst(سچ);
                    اكسر;
                }
            }
        }
        لكل (Ast_ڳنڍ_اظهار *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ڳنڍ_اظهار);

            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_ڳنڍ_اظهار* ثابت subargp = fargp ? fargp->exprp() : argp;
            إذا (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ثابت قدر_Varp = create_عارضي(subargp);
                قدر_Varp->noSubst(سچ);
                إذا (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ثابت nameVarp = create_عارضي(fargp->namep());
                    nameVarp->noSubst(سچ);
                }
                استمر;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            إذا (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ثابت varp = create_عارضي(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(سچ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    فراغ گهمو(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::جوڙيو_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_منزل_Const_Pool);
        V3Stats::جوڙيو_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

فراغ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("premit", 0, dump_وڻ_Either_Level() >= 3);
}
