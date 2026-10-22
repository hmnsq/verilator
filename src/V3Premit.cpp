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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Premit.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr પૂર્ણાંક STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

વર્ગ PremitVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    અચલ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ગાંઠ* m_stmtp = nullptr;  // Current statement
    તાર્કિક m_assignLhs = ખોટું;  // Inside assignment lhs, don't breakup extracts
    તાર્કિક m_inCons = ખોટું;  // Inside a NodeCons, so not the root of a value

    // METHODS
    તાર્કિક needsTemp(Ast_ગાંઠ_પદાવલિ* nodep) {
        // Consider adding a temp for this expression.
        જો (!m_stmtp) પાછા ખોટું;  // Not under a statement
        જો (nodep->user1SetOnce()) પાછા ખોટું;  // Already processed
        જો (!nodep->isWide()) પાછા ખોટું;  // Not wide
        જો (m_assignLhs) પાછા ખોટું;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        પાછા સાચું;
    }

    શૂન્ય check_ગાંઠ(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    સ્થિર તાર્કિક useConstPool(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
        uint64_t width = 0;
        તાર્કિક hasData = ખોટું;
        તાર્કિક hasDynamic = ખોટું;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        અચલ તાર્કિક emittable = nodep->forall([&](અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
            જો (VN_IS(exprp, ગાંઠ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                પાછા સાચું;
            }
            અચલ AstConst* અચલ constp = VN_CAST(exprp, Const);
            જો (!constp) પાછા ખોટું;
            width += static_cast<uint64_t>(constp->width_શબ્દો()) * VL_EDATASIZE;
            hasData = સાચું;
            પાછા !constp->અંક().isFourState();
        });
        જો (!emittable) પાછા ખોટું;
        // Dynamically sized values allocate when built with data, so use the pool for them
        જો (hasDynamic) પાછા hasData;
        પાછા width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_ગાંઠ_પદાવલિ* nodep) {
        // If enclosing statement is an assign
        Ast_ગાંઠ_Assign* અચલ assignp = VN_CAST(m_stmtp, ગાંઠ_Assign);
        જો (!assignp) પાછા nullptr;
        // of which 'nodep' is the RHS of
        જો (assignp->rhsp() != nodep) પાછા nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* અચલ lRefp = VN_CAST(assignp->lhsp(), VarRef);
        જો (!lRefp) પાછા nullptr;
        AstVar* અચલ varp = lRefp->varp();
        // And the RHS does not use the same variable
        જો (nodep->exists([varp](અચલ AstVarRef* refp) { પાછા refp->varp() == varp; })) {
            પાછા nullptr;
        }
        // Then return the LHS reference
        પાછા lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ગાંઠ_પદાવલિ* nodep) {
        File_લીટી* અચલ flp = nodep->fileline();
        અચલ std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* અચલ varp = નવું AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(સાચું);
        varp->noReset(સાચું);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        પાછા varp;
    }

    AstVar* createTemp(Ast_ગાંઠ_પદાવલિ* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_લીટી* અચલ flp = nodep->fileline();
        AstVar* varp = nullptr;

        જો (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* અચલ refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } નહીંતર {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* અચલ refp = નવું AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* અચલ assignp = નવું AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(નવું AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        પાછા varp;
    }

    શૂન્ય replaceWithOvrShift(Ast_ગાંઠ_Biop* nodep) {
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* newp = nullptr;
        જો (VN_IS(nodep, ShiftL)) {
            newp = નવું AstShiftLOvr{flp, lhsp, rhsp};
        } નહીંતર જો (VN_IS(nodep, ShiftR)) {
            newp = નવું AstShiftROvr{flp, lhsp, rhsp};
        } નહીંતર {
            newp = નવું AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visitShift(Ast_ગાંઠ_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_લીટી* અચલ flp = nodep->fileline();
        જો (અચલ AstConst* અચલ shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            અચલ તાર્કિક isOversized
                = shiftp->અંક().mostSetBitP1() > 32
                  || (shiftp->અંક().toUInt() >= static_cast<uint32_t>(nodep->width()));
            જો (isOversized) {
                // If signed shift, replace with replicated MSB
                જો (VN_IS(nodep, ShiftRS)) {
                    Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_ગાંઠ_પદાવલિ* અચલ msbp = નવું AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(નવું AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
                // Unsigned. If pure, replace with zero
                જો (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        નવું AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
                // Impure. Keep shift
                જો (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    પાછા;
                }
            }
        } નહીંતર {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            જો (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                પાછા;
            }
        }
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }

    સ્થિર તાર્કિક rhsReadsLhs(અચલ Ast_ગાંઠ_Assign* nodep) {
        અચલ VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](અચલ AstVarRef* refp) {
            જો (refp->access().isWriteOrRW()) refp->varp()->user3(સાચું);
        });
        પાછા nodep->rhsp()->exists([](અચલ AstVarRef* refp) {
            પાછા refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    શૂન્ય visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    જો (!m_cfuncp) પાછા; \
    જો ((stmtp)->user1SetOnce()) પાછા; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = ખોટું; \
    m_stmtp = (stmtp);

    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        જો (AstCvtArrayToPacked* અચલ packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ગાંઠ_પદાવલિ* અચલ પદાવલિ_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(પદાવલિ_Lhsp);
            VL_DO_DANGLING(ધકેલો_Deletep(packedp), packedp);
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(નવું AstCvtPackedToArray{rhsp->fileline(), rhsp, પદાવલિ_Lhsp->dtypep()});
            nodep->dtypeFrom(પદાવલિ_Lhsp);
        }
        // Direct assignment to a simple variable
        જો (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ગાંઠ* અચલ rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            જો (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) પાછા;
            જો (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } નહીંતર જો (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(સાચું);
            } નહીંતર જો (rhsp->width_શબ્દો() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                પાછા;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        અચલ તાર્કિક isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        જો (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } નહીંતર {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = સાચું;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        જો (AstVarRef* અચલ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            જો (AstVarRef* અચલ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                જો (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    શૂન્ય visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        જો (v3Global.opt.autoflush()) {
            અચલ Ast_ગાંઠ* searchp = nodep->nextp();
            જ્યારે (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            જો (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_વૃક્ષ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } નહીંતર {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    નવું AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_વૃક્ષ_Pure(સાચું) : nullptr});
            }
        }
    }
    શૂન્ય visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        જો (અચલ AstVarRef* અચલ refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(સાચું);
        } નહીંતર {
            createTemp(nodep->memp())->noSubst(સાચું);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    શૂન્ય visit(AstShiftL* nodep) override { visitShift(nodep); }
    શૂન્ય visit(AstShiftR* nodep) override { visitShift(nodep); }
    શૂન્ય visit(AstShiftRS* nodep) override { visitShift(nodep); }

    શૂન્ય visit(AstConst* nodep) override { check_ગાંઠ(nodep); }
    // Operators
    શૂન્ય visit(Ast_ગાંઠ_Termop* nodep) override { check_ગાંઠ(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
        જો (!VN_IS(nodep->backp(), ગાંઠ_Assign)) createTemp(nodep);
    }
    શૂન્ય visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ખોટું;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ખોટું;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    શૂન્ય visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ખોટું;
            iterateAndNextNull(nodep->bitp());
        }
        check_ગાંઠ(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        જો (!m_inCons && nodep->is_Constant_વૃક્ષ()) {
            જો (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            પાછા;
        }
        VL_RESTORER(m_inCons);
        m_inCons = સાચું;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    શૂન્ય visit(AstConsAssocItem* nodep) override {
        જો (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    શૂન્ય visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        જો (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* અચલ varp = createTemp(nodep);
            varp->noSubst(સાચું);  // Do not re-inline in V3Subst
        }
    }
    શૂન્ય visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        જો (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* અચલ lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(સાચું);

            File_લીટી* અચલ flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* અચલ thenRefp = નવું AstVarRef{flp, varp, VAccess::WRITE};
            જો (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_ગાંઠ_પદાવલિ* અચલ thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* અચલ thenAsspp = નવું AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* અચલ elseRefp = નવું AstVarRef{flp, varp, VAccess::WRITE};
            જો (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_ગાંઠ_પદાવલિ* અચલ elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* અચલ elseAsspp = નવું AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ગાંઠ_પદાવલિ* અચલ condp = nodep->condp()->unlinkFrBack();
            AstIf* અચલ ifp = નવું AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_વૃક્ષ();
            // Iterate the resulting assignments
            iterate(ifp);
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ગાંઠ_પદાવલિ* exprsp = nodep->exprsp();
        જો (nodep->પદાવલિ_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ગાંઠ_પદાવલિ);
            માટે (Ast_ગાંઠ* argp = exprsp; argp; argp = argp->nextp()) {
                અચલ AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
                જો (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* અચલ varp = createTemp(nodep->exprsp());
                    varp->noSubst(સાચું);
                    તોડો;
                }
            }
        }
        માટે (Ast_ગાંઠ_પદાવલિ *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ગાંઠ_પદાવલિ);

            AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
            Ast_ગાંઠ_પદાવલિ* અચલ subargp = fargp ? fargp->exprp() : argp;
            જો (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* અચલ કિંમત_Varp = createTemp(subargp);
                કિંમત_Varp->noSubst(સાચું);
                જો (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* અચલ nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(સાચું);
                }
                ચાલુ;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            જો (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* અચલ varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(સાચું);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    શૂન્ય visit(AstVar*) override {}  // Don't hit varrefs under vars
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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

શૂન્ય V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("premit", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
