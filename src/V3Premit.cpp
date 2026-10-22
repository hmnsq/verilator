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

constexpr صحیح STATIC_CONST_MIN_چوڑائی = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

جماعت PremitVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_مقصد_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current block
    سائز_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_گرہ* m_stmtp = خالی_اشاریہ;  // Current statement
    منطقی m_تفویض_Lhs = جھوٹ;  // Inside assignment lhs, don't breakup extracts
    منطقی m_اندر_Cons = جھوٹ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    منطقی needs_عارضی(Ast_گرہ_اظہاریہ* nodep) {
        // Consider adding a temp for this expression.
        اگر (!m_stmtp) واپس جھوٹ;  // Not under a statement
        اگر (nodep->user1_سیٹ_Once()) واپس جھوٹ;  // Already processed
        اگر (!nodep->isWide()) واپس جھوٹ;  // Not wide
        اگر (m_تفویض_Lhs) واپس جھوٹ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->پہلا_Abovep(), ArraySel), nodep, "Should have been ignored");
        واپس سچ;
    }

    خالی check_گرہ(Ast_گرہ_اظہاریہ* nodep) {
        اگر (needs_عارضی(nodep)) create_عارضی(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ساکن منطقی useConstPool(ثابت Ast_گرہ_اظہاریہ* nodep) {
        uint64_t چوڑائی = 0;
        منطقی has_ڈیٹا = جھوٹ;
        منطقی hasDynamic = جھوٹ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ثابت منطقی emittable = nodep->forall([&](ثابت Ast_گرہ_اظہاریہ* exprp) {
            اگر (VN_IS(exprp, گرہ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                واپس سچ;
            }
            ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
            اگر (!constp) واپس جھوٹ;
            چوڑائی += static_cast<uint64_t>(constp->چوڑائی_الفاظ()) * VL_EDATASIZE;
            has_ڈیٹا = سچ;
            واپس !constp->عدد().isFourState();
        });
        اگر (!emittable) واپس جھوٹ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        اگر (hasDynamic) واپس has_ڈیٹا;
        واپس چوڑائی >= static_cast<uint64_t>(STATIC_CONST_MIN_چوڑائی);
    }

    AstVarRef* is_Rhs_کا_تفویض_مقصد_Var(Ast_گرہ_اظہاریہ* nodep) {
        // If enclosing statement is an assign
        Ast_گرہ_تفویض* ثابت assignp = VN_CAST(m_stmtp, گرہ_تفویض);
        اگر (!assignp) واپس خالی_اشاریہ;
        // of which 'nodep' is the RHS of
        اگر (assignp->rhsp() != nodep) واپس خالی_اشاریہ;
        // and the LHS is a sipmple variable reference
        AstVarRef* ثابت lRefp = VN_CAST(assignp->lhsp(), VarRef);
        اگر (!lRefp) واپس خالی_اشاریہ;
        AstVar* ثابت varp = lRefp->varp();
        // And the RHS does not use the same variable
        اگر (nodep->exists([varp](ثابت AstVarRef* refp) { واپس refp->varp() == varp; })) {
            واپس خالی_اشاریہ;
        }
        // Then return the LHS reference
        واپس lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_گرہ_اظہاریہ* nodep) {
        File_سطر* ثابت flp = nodep->fileline();
        ثابت std::string name = "__Vtemp_" + std::مقصد_string(++m_tmpVarCnt);
        AstVar* ثابت varp = نیا AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(سچ);
        varp->noReset(سچ);
        m_cfuncp->جوڑو_Varsp(varp);
        ++m_temporaryVarsCreated;
        واپس varp;
    }

    AstVar* create_عارضی(Ast_گرہ_اظہاریہ* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_عقب(&relinker);

        File_سطر* ثابت flp = nodep->fileline();
        AstVar* varp = خالی_اشاریہ;

        اگر (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ثابت refp = V3ConstPool::ڈھونڈو(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            ++m_extracted_مقصد_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ورنہ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ثابت refp = نیا AstVarRef{flp, varp, VAccess::WRITE};
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->جوڑو_یہاں_This_As_اگلا(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(نیا AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        واپس varp;
    }

    خالی replace_With_Ovr_سرکاؤ(Ast_گرہ_Biop* nodep) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* newp = خالی_اشاریہ;
        اگر (VN_IS(nodep, سرکاؤ_L)) {
            newp = نیا Ast_سرکاؤ_LOvr{flp, lhsp, rhsp};
        } ورنہ اگر (VN_IS(nodep, سرکاؤ_R)) {
            newp = نیا Ast_سرکاؤ_ROvr{flp, lhsp, rhsp};
        } ورنہ {
            newp = نیا Ast_سرکاؤ_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ_سرکاؤ(Ast_گرہ_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, سرکاؤ_L) || VN_IS(nodep, سرکاؤ_R) || VN_IS(nodep, سرکاؤ_RS), nodep,
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
                = shiftp->عدد().most_سیٹ_ثنائی_ہندسہ_P1() > 32
                  || (shiftp->عدد().مقصد_UInt() >= static_cast<uint32_t>(nodep->چوڑائی()));
            اگر (isOversized) {
                // If signed shift, replace with replicated MSB
                اگر (VN_IS(nodep, سرکاؤ_RS)) {
                    Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
                    Ast_گرہ_اظہاریہ* ثابت msbp = نیا AstSel{flp, lhsp, lhsp->چوڑائی_Min() - 1, 1};
                    nodep->replaceWithKeepDType(نیا AstExtendS{flp, msbp, nodep->چوڑائی()});
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
                // Unsigned. If pure, replace with zero
                اگر (nodep->lhsp()->is_خالص()) {
                    nodep->replaceWithKeepDType(
                        نیا AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس;
                }
                // Impure. Keep shift
                اگر (!nodep->isWide()) {
                    replace_With_Ovr_سرکاؤ(nodep);
                    واپس;
                }
            }
        } ورنہ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            اگر (!nodep->isWide() && (nodep->چوڑائی() < (1LL << nodep->rhsp()->چوڑائی_Min()))) {
                replace_With_Ovr_سرکاؤ(nodep);
                واپس;
            }
        }
        iterateChildren(nodep);
        check_گرہ(nodep);
    }

    ساکن منطقی rhsReadsLhs(ثابت Ast_گرہ_تفویض* nodep) {
        ثابت VNUser3_اندر_Use user3_اندر_Use;
        nodep->lhsp()->foreach([](ثابت AstVarRef* refp) {
            اگر (refp->access().isWriteOrRW()) refp->varp()->user3(سچ);
        });
        واپس nodep->rhsp()->exists([](ثابت AstVarRef* refp) {
            واپس refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#تعریف_کرو آغاز_STATEMENT_OR_RETURN(stmtp) \
    اگر (!m_cfuncp) واپس; \
    اگر ((stmtp)->user1_سیٹ_Once()) واپس; \
    VL_RESTORER(m_تفویض_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_تفویض_Lhs = جھوٹ; \
    m_stmtp = (stmtp);

    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        آغاز_STATEMENT_OR_RETURN(nodep);

        اگر (Ast_Cvt_Array_مقصد_Packed* ثابت packedp = VN_CAST(nodep->lhsp(), Cvt_Array_مقصد_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_گرہ_اظہاریہ* ثابت اظہاریہ_Lhsp = packedp->fromp()->unlink_Fr_عقب();
            packedp->replaceWith(اظہاریہ_Lhsp);
            VL_DO_DANGLING(ڈالو_Deletep(packedp), packedp);
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_عقب();
            nodep->rhsp(نیا Ast_Cvt_Packed_مقصد_Array{rhsp->fileline(), rhsp, اظہاریہ_Lhsp->dtypep()});
            nodep->dtypeFrom(اظہاریہ_Lhsp);
        }
        // Direct assignment to a simple variable
        اگر (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_گرہ* ثابت rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            اگر (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) واپس;
            اگر (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ورنہ اگر (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(سچ);
            } ورنہ اگر (rhsp->چوڑائی_الفاظ() * VL_EDATASIZE < STATIC_CONST_MIN_چوڑائی) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                واپس;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ثابت منطقی isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        اگر (!isAtomic && rhsReadsLhs(nodep)) {
            create_عارضی(nodep->rhsp());
        } ورنہ {
            iterate_And_اگلا_Null(nodep->rhsp());
        }

        m_تفویض_Lhs = سچ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_اگلا_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        اگر (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            اگر (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                اگر (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                }
            }
        }
    }
    خالی دورہ(AstDisplay* nodep) override {
        آغاز_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        اگر (v3Global.opt.autoflush()) {
            ثابت Ast_گرہ* searchp = nodep->nextp();
            جب (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            اگر (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_درخت(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ورنہ {
                UINFO(4, "Autoflush " << nodep);
                nodep->جوڑو_اگلا_یہاں(
                    نیا AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_درخت_خالص(سچ) : خالی_اشاریہ});
            }
        }
    }
    خالی دورہ(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        آغاز_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(سچ);
        } ورنہ {
            create_عارضی(nodep->memp())->noSubst(سچ);
        }
    }
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        آغاز_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#تعریف_منسوخ آغاز_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    خالی دورہ(Ast_سرکاؤ_L* nodep) override { دورہ_سرکاؤ(nodep); }
    خالی دورہ(Ast_سرکاؤ_R* nodep) override { دورہ_سرکاؤ(nodep); }
    خالی دورہ(Ast_سرکاؤ_RS* nodep) override { دورہ_سرکاؤ(nodep); }

    خالی دورہ(AstConst* nodep) override { check_گرہ(nodep); }
    // Operators
    خالی دورہ(Ast_گرہ_Termop* nodep) override { check_گرہ(nodep); }
    خالی دورہ(Ast_گرہ_Uniop* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(Ast_گرہ_Biop* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(AstRand* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(Ast_Cvt_Array_مقصد_Packed* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(Ast_Cvt_Packed_مقصد_Array* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
        اگر (!VN_IS(nodep->backp(), گرہ_تفویض)) create_عارضی(nodep);
    }
    خالی دورہ(Ast_Cvt_Unpack_مقصد_قطار* nodep) override {
        iterateChildren(nodep);
        check_گرہ(nodep);
    }
    خالی دورہ(AstSel* nodep) override {
        iterate_And_اگلا_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_تفویض_Lhs);
            m_تفویض_Lhs = جھوٹ;
            iterate_And_اگلا_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_گرہ(nodep);
    }
    خالی دورہ(AstArraySel* nodep) override {
        iterate_And_اگلا_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_تفویض_Lhs);
            m_تفویض_Lhs = جھوٹ;
            iterate_And_اگلا_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    خالی دورہ(AstAssocSel* nodep) override {
        iterate_And_اگلا_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_تفویض_Lhs);
            m_تفویض_Lhs = جھوٹ;
            iterate_And_اگلا_Null(nodep->bitp());
        }
        check_گرہ(nodep);
    }
    خالی دورہ(Ast_گرہ_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        اگر (!m_اندر_Cons && nodep->is_Constant_درخت()) {
            اگر (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::ڈھونڈو(nodep));
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                ++m_extracted_مقصد_Const_Pool;
            }
            واپس;
        }
        VL_RESTORER(m_اندر_Cons);
        m_اندر_Cons = سچ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    خالی دورہ(Ast_Cons_Assoc_شے* nodep) override {
        اگر (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    خالی دورہ(Ast_مطابقت_Masked* nodep) override {
        iterateChildren(nodep);
        اگر (!nodep->user1_سیٹ_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* ثابت varp = create_عارضی(nodep);
            varp->noSubst(سچ);  // Do not re-inline in V3Subst
        }
    }
    خالی دورہ(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        اگر (needs_عارضی(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ثابت lRefp = is_Rhs_کا_تفویض_مقصد_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(سچ);

            File_سطر* ثابت flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ثابت thenRefp = نیا AstVarRef{flp, varp, VAccess::WRITE};
            اگر (lRefp) thenRefp->self_اشارہ_گر(lRefp->self_اشارہ_گر());
            Ast_گرہ_اظہاریہ* ثابت thenExprp = nodep->thenp()->unlink_Fr_عقب();
            Ast_تفویض* ثابت thenAsspp = نیا Ast_تفویض{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ثابت elseRefp = نیا AstVarRef{flp, varp, VAccess::WRITE};
            اگر (lRefp) elseRefp->self_اشارہ_گر(lRefp->self_اشارہ_گر());
            Ast_گرہ_اظہاریہ* ثابت elseExprp = nodep->elsep()->unlink_Fr_عقب();
            Ast_تفویض* ثابت elseAsspp = نیا Ast_تفویض{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_گرہ_اظہاریہ* ثابت condp = nodep->condp()->unlink_Fr_عقب();
            AstIf* ثابت ifp = نیا AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->جوڑو_یہاں_This_As_اگلا(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_درخت();
            // Iterate the resulting assignments
            iterate(ifp);
            واپس;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_گرہ_اظہاریہ* exprsp = nodep->exprsp();
        اگر (nodep->اظہاریہ_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), گرہ_اظہاریہ);
            برائے (Ast_گرہ* argp = exprsp; argp; argp = argp->nextp()) {
                ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                اگر (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ثابت varp = create_عارضی(nodep->exprsp());
                    varp->noSubst(سچ);
                    توڑو;
                }
            }
        }
        برائے (Ast_گرہ_اظہاریہ *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), گرہ_اظہاریہ);

            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_گرہ_اظہاریہ* ثابت subargp = fargp ? fargp->exprp() : argp;
            اگر (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ثابت قدر_Varp = create_عارضی(subargp);
                قدر_Varp->noSubst(سچ);
                اگر (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ثابت nameVarp = create_عارضی(fargp->namep());
                    nameVarp->noSubst(سچ);
                }
                جاری;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            اگر (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ثابت varp = create_عارضی(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(سچ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    خالی دورہ(AstVar*) override {}  // Don't hit varrefs under vars
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::جوڑو_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_مقصد_Const_Pool);
        V3Stats::جوڑو_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

خالی V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("premit", 0, dump_درخت_Either_Level() >= 3);
}
