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

constexpr صحيح STATIC_CONST_MIN_پلنوالی = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

صنف PremitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_منزل_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = تش_اشاره;  // Current block
    اندازه_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_غوټه* m_stmtp = تش_اشاره;  // Current statement
    منطقي m_ټاکل_Lhs = خطأ;  // Inside assignment lhs, don't breakup extracts
    منطقي m_دننه_Cons = خطأ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    منطقي needs_لنډمهاله(Ast_غوټه_بیان* nodep) {
        // Consider adding a temp for this expression.
        إذا (!m_stmtp) أرجع خطأ;  // Not under a statement
        إذا (nodep->user1_سېټ_Once()) أرجع خطأ;  // Already processed
        إذا (!nodep->isWide()) أرجع خطأ;  // Not wide
        إذا (m_ټاکل_Lhs) أرجع خطأ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->لومړی_Abovep(), ArraySel), nodep, "Should have been ignored");
        أرجع ریښتیا;
    }

    فراغ check_غوټه(Ast_غوټه_بیان* nodep) {
        إذا (needs_لنډمهاله(nodep)) create_لنډمهاله(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ساكن منطقي useConstPool(ثابت Ast_غوټه_بیان* nodep) {
        uint64_t پلنوالی = 0;
        منطقي has_معلومات = خطأ;
        منطقي hasDynamic = خطأ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ثابت منطقي emittable = nodep->forall([&](ثابت Ast_غوټه_بیان* exprp) {
            إذا (VN_IS(exprp, غوټه_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                أرجع ریښتیا;
            }
            ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
            إذا (!constp) أرجع خطأ;
            پلنوالی += static_cast<uint64_t>(constp->پلنوالی_ټکي()) * VL_EDATASIZE;
            has_معلومات = ریښتیا;
            أرجع !constp->عدد().isFourState();
        });
        إذا (!emittable) أرجع خطأ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        إذا (hasDynamic) أرجع has_معلومات;
        أرجع پلنوالی >= static_cast<uint64_t>(STATIC_CONST_MIN_پلنوالی);
    }

    AstVarRef* is_Rhs_د_ټاکل_منزل_Var(Ast_غوټه_بیان* nodep) {
        // If enclosing statement is an assign
        Ast_غوټه_ټاکل* ثابت assignp = VN_CAST(m_stmtp, غوټه_ټاکل);
        إذا (!assignp) أرجع تش_اشاره;
        // of which 'nodep' is the RHS of
        إذا (assignp->rhsp() != nodep) أرجع تش_اشاره;
        // and the LHS is a sipmple variable reference
        AstVarRef* ثابت lRefp = VN_CAST(assignp->lhsp(), VarRef);
        إذا (!lRefp) أرجع تش_اشاره;
        AstVar* ثابت varp = lRefp->varp();
        // And the RHS does not use the same variable
        إذا (nodep->exists([varp](ثابت AstVarRef* refp) { أرجع refp->varp() == varp; })) {
            أرجع تش_اشاره;
        }
        // Then return the LHS reference
        أرجع lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_غوټه_بیان* nodep) {
        File_کرښه* ثابت flp = nodep->fileline();
        ثابت std::string name = "__Vtemp_" + std::منزل_string(++m_tmpVarCnt);
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(ریښتیا);
        varp->noReset(ریښتیا);
        m_cfuncp->جمع_کړه_Varsp(varp);
        ++m_temporaryVarsCreated;
        أرجع varp;
    }

    AstVar* create_لنډمهاله(Ast_غوټه_بیان* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_شا(&relinker);

        File_کرښه* ثابت flp = nodep->fileline();
        AstVar* varp = تش_اشاره;

        إذا (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ثابت refp = V3ConstPool::پیدا_کړه(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            ++m_extracted_منزل_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } وإلا {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ثابت refp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->جمع_کړه_دلته_This_As_بل(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(جديد AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        أرجع varp;
    }

    فراغ replace_With_Ovr_ښويدنه(Ast_غوټه_Biop* nodep) {
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* newp = تش_اشاره;
        إذا (VN_IS(nodep, ښويدنه_L)) {
            newp = جديد Ast_ښويدنه_LOvr{flp, lhsp, rhsp};
        } وإلا إذا (VN_IS(nodep, ښويدنه_R)) {
            newp = جديد Ast_ښويدنه_ROvr{flp, lhsp, rhsp};
        } وإلا {
            newp = جديد Ast_ښويدنه_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه_ښويدنه(Ast_غوټه_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ښويدنه_L) || VN_IS(nodep, ښويدنه_R) || VN_IS(nodep, ښويدنه_RS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_کرښه* ثابت flp = nodep->fileline();
        إذا (ثابت AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ثابت منطقي isOversized
                = shiftp->عدد().most_سېټ_بټ_P1() > 32
                  || (shiftp->عدد().منزل_UInt() >= static_cast<uint32_t>(nodep->پلنوالی()));
            إذا (isOversized) {
                // If signed shift, replace with replicated MSB
                إذا (VN_IS(nodep, ښويدنه_RS)) {
                    Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
                    Ast_غوټه_بیان* ثابت msbp = جديد AstSel{flp, lhsp, lhsp->پلنوالی_Min() - 1, 1};
                    nodep->replaceWithKeepDType(جديد AstExtendS{flp, msbp, nodep->پلنوالی()});
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
                // Unsigned. If pure, replace with zero
                إذا (nodep->lhsp()->is_خالص()) {
                    nodep->replaceWithKeepDType(
                        جديد AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
                // Impure. Keep shift
                إذا (!nodep->isWide()) {
                    replace_With_Ovr_ښويدنه(nodep);
                    أرجع;
                }
            }
        } وإلا {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            إذا (!nodep->isWide() && (nodep->پلنوالی() < (1LL << nodep->rhsp()->پلنوالی_Min()))) {
                replace_With_Ovr_ښويدنه(nodep);
                أرجع;
            }
        }
        iterateChildren(nodep);
        check_غوټه(nodep);
    }

    ساكن منطقي rhsReadsLhs(ثابت Ast_غوټه_ټاکل* nodep) {
        ثابت VNUser3_دننه_Use user3_دننه_Use;
        nodep->lhsp()->foreach([](ثابت AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) refp->varp()->user3(ریښتیا);
        });
        أرجع nodep->rhsp()->exists([](ثابت AstVarRef* refp) {
            أرجع refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    فراغ لیدنه(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#تعریف_کړه پیل_STATEMENT_OR_RETURN(stmtp) \
    إذا (!m_cfuncp) أرجع; \
    إذا ((stmtp)->user1_سېټ_Once()) أرجع; \
    VL_RESTORER(m_ټاکل_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_ټاکل_Lhs = خطأ; \
    m_stmtp = (stmtp);

    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        پیل_STATEMENT_OR_RETURN(nodep);

        إذا (Ast_Cvt_Array_منزل_Packed* ثابت packedp = VN_CAST(nodep->lhsp(), Cvt_Array_منزل_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_غوټه_بیان* ثابت بیان_Lhsp = packedp->fromp()->unlink_Fr_شا();
            packedp->replaceWith(بیان_Lhsp);
            VL_DO_DANGLING(ټېل_Deletep(packedp), packedp);
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            nodep->rhsp(جديد Ast_Cvt_Packed_منزل_Array{rhsp->fileline(), rhsp, بیان_Lhsp->dtypep()});
            nodep->dtypeFrom(بیان_Lhsp);
        }
        // Direct assignment to a simple variable
        إذا (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_غوټه* ثابت rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            إذا (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) أرجع;
            إذا (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } وإلا إذا (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(ریښتیا);
            } وإلا إذا (rhsp->پلنوالی_ټکي() * VL_EDATASIZE < STATIC_CONST_MIN_پلنوالی) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                أرجع;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ثابت منطقي isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        إذا (!isAtomic && rhsReadsLhs(nodep)) {
            create_لنډمهاله(nodep->rhsp());
        } وإلا {
            iterate_And_بل_Null(nodep->rhsp());
        }

        m_ټاکل_Lhs = ریښتیا;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_بل_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            إذا (AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                إذا (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                }
            }
        }
    }
    فراغ لیدنه(AstDisplay* nodep) override {
        پیل_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        إذا (v3Global.opt.autoflush()) {
            ثابت Ast_غوټه* searchp = nodep->nextp();
            طالما (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            إذا (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_ونه(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } وإلا {
                UINFO(4, "Autoflush " << nodep);
                nodep->جمع_کړه_بل_دلته(
                    جديد AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_ونه_خالص(ریښتیا) : تش_اشاره});
            }
        }
    }
    فراغ لیدنه(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        پیل_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(ریښتیا);
        } وإلا {
            create_لنډمهاله(nodep->memp())->noSubst(ریښتیا);
        }
    }
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        پیل_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#تعریف_لغوه پیل_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    فراغ لیدنه(Ast_ښويدنه_L* nodep) override { لیدنه_ښويدنه(nodep); }
    فراغ لیدنه(Ast_ښويدنه_R* nodep) override { لیدنه_ښويدنه(nodep); }
    فراغ لیدنه(Ast_ښويدنه_RS* nodep) override { لیدنه_ښويدنه(nodep); }

    فراغ لیدنه(AstConst* nodep) override { check_غوټه(nodep); }
    // Operators
    فراغ لیدنه(Ast_غوټه_Termop* nodep) override { check_غوټه(nodep); }
    فراغ لیدنه(Ast_غوټه_Uniop* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(AstRand* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(Ast_Cvt_Array_منزل_Packed* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(Ast_Cvt_Packed_منزل_Array* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
        إذا (!VN_IS(nodep->backp(), غوټه_ټاکل)) create_لنډمهاله(nodep);
    }
    فراغ لیدنه(Ast_Cvt_Unpack_منزل_کتار* nodep) override {
        iterateChildren(nodep);
        check_غوټه(nodep);
    }
    فراغ لیدنه(AstSel* nodep) override {
        iterate_And_بل_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ټاکل_Lhs);
            m_ټاکل_Lhs = خطأ;
            iterate_And_بل_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_غوټه(nodep);
    }
    فراغ لیدنه(AstArraySel* nodep) override {
        iterate_And_بل_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ټاکل_Lhs);
            m_ټاکل_Lhs = خطأ;
            iterate_And_بل_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    فراغ لیدنه(AstAssocSel* nodep) override {
        iterate_And_بل_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ټاکل_Lhs);
            m_ټاکل_Lhs = خطأ;
            iterate_And_بل_Null(nodep->bitp());
        }
        check_غوټه(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        إذا (!m_دننه_Cons && nodep->is_Constant_ونه()) {
            إذا (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::پیدا_کړه(nodep));
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                ++m_extracted_منزل_Const_Pool;
            }
            أرجع;
        }
        VL_RESTORER(m_دننه_Cons);
        m_دننه_Cons = ریښتیا;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    فراغ لیدنه(Ast_Cons_Assoc_توکی* nodep) override {
        إذا (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    فراغ لیدنه(Ast_سمون_Masked* nodep) override {
        iterateChildren(nodep);
        إذا (!nodep->user1_سېټ_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* ثابت varp = create_لنډمهاله(nodep);
            varp->noSubst(ریښتیا);  // Do not re-inline in V3Subst
        }
    }
    فراغ لیدنه(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        إذا (needs_لنډمهاله(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ثابت lRefp = is_Rhs_د_ټاکل_منزل_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(ریښتیا);

            File_کرښه* ثابت flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ثابت thenRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            إذا (lRefp) thenRefp->self_اشاره_ګر(lRefp->self_اشاره_ګر());
            Ast_غوټه_بیان* ثابت thenExprp = nodep->thenp()->unlink_Fr_شا();
            Ast_ټاکل* ثابت thenAsspp = جديد Ast_ټاکل{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ثابت elseRefp = جديد AstVarRef{flp, varp, VAccess::WRITE};
            إذا (lRefp) elseRefp->self_اشاره_ګر(lRefp->self_اشاره_ګر());
            Ast_غوټه_بیان* ثابت elseExprp = nodep->elsep()->unlink_Fr_شا();
            Ast_ټاکل* ثابت elseAsspp = جديد Ast_ټاکل{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_غوټه_بیان* ثابت condp = nodep->condp()->unlink_Fr_شا();
            AstIf* ثابت ifp = جديد AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->جمع_کړه_دلته_This_As_بل(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_ونه();
            // Iterate the resulting assignments
            iterate(ifp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_غوټه_بیان* exprsp = nodep->exprsp();
        إذا (nodep->بیان_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), غوټه_بیان);
            لكل (Ast_غوټه* argp = exprsp; argp; argp = argp->nextp()) {
                ثابت AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                إذا (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ثابت varp = create_لنډمهاله(nodep->exprsp());
                    varp->noSubst(ریښتیا);
                    اكسر;
                }
            }
        }
        لكل (Ast_غوټه_بیان *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), غوټه_بیان);

            AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
            Ast_غوټه_بیان* ثابت subargp = fargp ? fargp->exprp() : argp;
            إذا (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ثابت ارزښت_Varp = create_لنډمهاله(subargp);
                ارزښت_Varp->noSubst(ریښتیا);
                إذا (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ثابت nameVarp = create_لنډمهاله(fargp->namep());
                    nameVarp->noSubst(ریښتیا);
                }
                استمر;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            إذا (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ثابت varp = create_لنډمهاله(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(ریښتیا);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    فراغ لیدنه(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::جمع_کړه_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_منزل_Const_Pool);
        V3Stats::جمع_کړه_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

فراغ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("premit", 0, dump_ونه_Either_Level() >= 3);
}
