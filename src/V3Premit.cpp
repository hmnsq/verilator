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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Premit.h"

#समावेश "V3ConstPool.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr पूर्णांक STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

श्रेणी PremitVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    स्थिर VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    माप_प्रकार m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_गाँठ* m_stmtp = nullptr;  // Current statement
    bool m_assignLhs = false;  // Inside assignment lhs, don't breakup extracts
    bool m_inCons = false;  // Inside a NodeCons, so not the root of a value

    // METHODS
    bool needs_अस्थायी(Ast_गाँठ_व्यंजक* nodep) {
        // Consider adding a temp for this expression.
        अगर (!m_stmtp) वापस false;  // Not under a statement
        अगर (nodep->user1SetOnce()) वापस false;  // Already processed
        अगर (!nodep->isWide()) वापस false;  // Not wide
        अगर (m_assignLhs) वापस false;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        वापस true;
    }

    व्योम check_गाँठ(Ast_गाँठ_व्यंजक* nodep) {
        अगर (needs_अस्थायी(nodep)) create_अस्थायी(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    अटल bool useConstPool(स्थिर Ast_गाँठ_व्यंजक* nodep) {
        uint64_t width = 0;
        bool has_तथ्य = false;
        bool hasDynamic = false;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        स्थिर bool emittable = nodep->forall([&](स्थिर Ast_गाँठ_व्यंजक* exprp) {
            अगर (VN_IS(exprp, गाँठ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                वापस true;
            }
            स्थिर AstConst* स्थिर constp = VN_CAST(exprp, Const);
            अगर (!constp) वापस false;
            width += static_cast<uint64_t>(constp->width_शब्दगण()) * VL_EDATASIZE;
            has_तथ्य = true;
            वापस !constp->अंक().isFourState();
        });
        अगर (!emittable) वापस false;
        // Dynamically sized values allocate when built with data, so use the pool for them
        अगर (hasDynamic) वापस has_तथ्य;
        वापस width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_गाँठ_व्यंजक* nodep) {
        // If enclosing statement is an assign
        Ast_गाँठ_Assign* स्थिर assignp = VN_CAST(m_stmtp, गाँठ_Assign);
        अगर (!assignp) वापस nullptr;
        // of which 'nodep' is the RHS of
        अगर (assignp->rhsp() != nodep) वापस nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* स्थिर lRefp = VN_CAST(assignp->lhsp(), VarRef);
        अगर (!lRefp) वापस nullptr;
        AstVar* स्थिर varp = lRefp->varp();
        // And the RHS does not use the same variable
        अगर (nodep->exists([varp](स्थिर AstVarRef* refp) { वापस refp->varp() == varp; })) {
            वापस nullptr;
        }
        // Then return the LHS reference
        वापस lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_गाँठ_व्यंजक* nodep) {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        स्थिर std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* स्थिर varp = नया AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(true);
        varp->noReset(true);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        वापस varp;
    }

    AstVar* create_अस्थायी(Ast_गाँठ_व्यंजक* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstVar* varp = nullptr;

        अगर (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* स्थिर refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } अन्यथा {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* स्थिर refp = नया AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* स्थिर assignp = नया AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->add_Here_This_As_अग्र(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(नया AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        वापस varp;
    }

    व्योम replaceWithOvrShift(Ast_गाँठ_Biop* nodep) {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* newp = nullptr;
        अगर (VN_IS(nodep, ShiftL)) {
            newp = नया AstShiftLOvr{flp, lhsp, rhsp};
        } अन्यथा अगर (VN_IS(nodep, ShiftR)) {
            newp = नया AstShiftROvr{flp, lhsp, rhsp};
        } अन्यथा {
            newp = नया AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    व्योम visitShift(Ast_गाँठ_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_पंक्ति* स्थिर flp = nodep->fileline();
        अगर (स्थिर AstConst* स्थिर shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            स्थिर bool isOversized
                = shiftp->अंक().mostSetBitP1() > 32
                  || (shiftp->अंक().toUInt() >= static_cast<uint32_t>(nodep->width()));
            अगर (isOversized) {
                // If signed shift, replace with replicated MSB
                अगर (VN_IS(nodep, ShiftRS)) {
                    Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_गाँठ_व्यंजक* स्थिर msbp = नया AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(नया AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
                // Unsigned. If pure, replace with zero
                अगर (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        नया AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
                // Impure. Keep shift
                अगर (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    वापस;
                }
            }
        } अन्यथा {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            अगर (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                वापस;
            }
        }
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }

    अटल bool rhsReadsLhs(स्थिर Ast_गाँठ_Assign* nodep) {
        स्थिर VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](स्थिर AstVarRef* refp) {
            अगर (refp->access().isWriteOrRW()) refp->varp()->user3(true);
        });
        वापस nodep->rhsp()->exists([](स्थिर AstVarRef* refp) {
            वापस refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#घोषणा START_STATEMENT_OR_RETURN(stmtp) \
    अगर (!m_cfuncp) वापस; \
    अगर ((stmtp)->user1SetOnce()) वापस; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = false; \
    m_stmtp = (stmtp);

    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        अगर (AstCvtArrayToPacked* स्थिर packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_गाँठ_व्यंजक* स्थिर व्यंजक_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(व्यंजक_Lhsp);
            VL_DO_DANGLING(धकेलो_Deletep(packedp), packedp);
            Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(नया AstCvtPackedToArray{rhsp->fileline(), rhsp, व्यंजक_Lhsp->dtypep()});
            nodep->dtypeFrom(व्यंजक_Lhsp);
        }
        // Direct assignment to a simple variable
        अगर (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_गाँठ* स्थिर rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            अगर (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) वापस;
            अगर (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } अन्यथा अगर (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(true);
            } अन्यथा अगर (rhsp->width_शब्दगण() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                वापस;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        स्थिर bool isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        अगर (!isAtomic && rhsReadsLhs(nodep)) {
            create_अस्थायी(nodep->rhsp());
        } अन्यथा {
            iterate_And_अग्र_Null(nodep->rhsp());
        }

        m_assignLhs = true;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_अग्र_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        अगर (AstVarRef* स्थिर rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            अगर (AstVarRef* स्थिर lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                अगर (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    व्योम visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        अगर (v3Global.opt.autoflush()) {
            स्थिर Ast_गाँठ* searchp = nodep->nextp();
            जबतक (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            अगर (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_वृक्ष(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } अन्यथा {
                UINFO(4, "Autoflush " << nodep);
                nodep->add_अग्र_Here(
                    नया AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_वृक्ष_Pure(true) : nullptr});
            }
        }
    }
    व्योम visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(true);
        } अन्यथा {
            create_अस्थायी(nodep->memp())->noSubst(true);
        }
    }
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#अघोषित START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    व्योम visit(AstShiftL* nodep) override { visitShift(nodep); }
    व्योम visit(AstShiftR* nodep) override { visitShift(nodep); }
    व्योम visit(AstShiftRS* nodep) override { visitShift(nodep); }

    व्योम visit(AstConst* nodep) override { check_गाँठ(nodep); }
    // Operators
    व्योम visit(Ast_गाँठ_Termop* nodep) override { check_गाँठ(nodep); }
    व्योम visit(Ast_गाँठ_Uniop* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(Ast_गाँठ_Biop* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
        अगर (!VN_IS(nodep->backp(), गाँठ_Assign)) create_अस्थायी(nodep);
    }
    व्योम visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_गाँठ(nodep);
    }
    व्योम visit(AstSel* nodep) override {
        iterate_And_अग्र_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = false;
            iterate_And_अग्र_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_गाँठ(nodep);
    }
    व्योम visit(AstArraySel* nodep) override {
        iterate_And_अग्र_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = false;
            iterate_And_अग्र_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    व्योम visit(AstAssocSel* nodep) override {
        iterate_And_अग्र_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = false;
            iterate_And_अग्र_Null(nodep->bitp());
        }
        check_गाँठ(nodep);
    }
    व्योम visit(Ast_गाँठ_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        अगर (!m_inCons && nodep->is_Constant_वृक्ष()) {
            अगर (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            वापस;
        }
        VL_RESTORER(m_inCons);
        m_inCons = true;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    व्योम visit(AstConsAssocItem* nodep) override {
        अगर (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    व्योम visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        अगर (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* स्थिर varp = create_अस्थायी(nodep);
            varp->noSubst(true);  // Do not re-inline in V3Subst
        }
    }
    व्योम visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        अगर (needs_अस्थायी(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* स्थिर lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(true);

            File_पंक्ति* स्थिर flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* स्थिर thenRefp = नया AstVarRef{flp, varp, VAccess::WRITE};
            अगर (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_गाँठ_व्यंजक* स्थिर thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* स्थिर thenAsspp = नया AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* स्थिर elseRefp = नया AstVarRef{flp, varp, VAccess::WRITE};
            अगर (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_गाँठ_व्यंजक* स्थिर elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* स्थिर elseAsspp = नया AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_गाँठ_व्यंजक* स्थिर condp = nodep->condp()->unlinkFrBack();
            AstIf* स्थिर ifp = नया AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->add_Here_This_As_अग्र(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(नया AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_वृक्ष();
            // Iterate the resulting assignments
            iterate(ifp);
            वापस;
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_गाँठ_व्यंजक* exprsp = nodep->exprsp();
        अगर (nodep->व्यंजक_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), गाँठ_व्यंजक);
            क्रम (Ast_गाँठ* argp = exprsp; argp; argp = argp->nextp()) {
                स्थिर AstSFormatArg* स्थिर fargp = VN_CAST(argp, SFormatArg);
                अगर (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* स्थिर varp = create_अस्थायी(nodep->exprsp());
                    varp->noSubst(true);
                    अवरोध;
                }
            }
        }
        क्रम (Ast_गाँठ_व्यंजक *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), गाँठ_व्यंजक);

            AstSFormatArg* स्थिर fargp = VN_CAST(argp, SFormatArg);
            Ast_गाँठ_व्यंजक* स्थिर subargp = fargp ? fargp->exprp() : argp;
            अगर (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* स्थिर मूल्य_Varp = create_अस्थायी(subargp);
                मूल्य_Varp->noSubst(true);
                अगर (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* स्थिर nameVarp = create_अस्थायी(fargp->namep());
                    nameVarp->noSubst(true);
                }
                जारी;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            अगर (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* स्थिर varp = create_अस्थायी(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(true);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    व्योम visit(AstVar*) override {}  // Don't hit varrefs under vars
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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

व्योम V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("premit", 0, dump_वृक्ष_Either_Level() >= 3);
}
