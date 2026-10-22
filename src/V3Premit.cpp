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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Premit.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr முழு STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

வகுப்பு PremitVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    மாறிலி VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_முனை* m_stmtp = nullptr;  // Current statement
    தருக்க m_assignLhs = பொய்;  // Inside assignment lhs, don't breakup extracts
    தருக்க m_inCons = பொய்;  // Inside a NodeCons, so not the root of a value

    // METHODS
    தருக்க needsTemp(Ast_முனை_கோவை* nodep) {
        // Consider adding a temp for this expression.
        எனில் (!m_stmtp) திரும்பு பொய்;  // Not under a statement
        எனில் (nodep->user1SetOnce()) திரும்பு பொய்;  // Already processed
        எனில் (!nodep->isWide()) திரும்பு பொய்;  // Not wide
        எனில் (m_assignLhs) திரும்பு பொய்;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        திரும்பு உண்மை;
    }

    வெறுமை check_முனை(Ast_முனை_கோவை* nodep) {
        எனில் (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    நிலையான தருக்க useConstPool(மாறிலி Ast_முனை_கோவை* nodep) {
        uint64_t width = 0;
        தருக்க hasData = பொய்;
        தருக்க hasDynamic = பொய்;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        மாறிலி தருக்க emittable = nodep->forall([&](மாறிலி Ast_முனை_கோவை* exprp) {
            எனில் (VN_IS(exprp, முனை_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                திரும்பு உண்மை;
            }
            மாறிலி AstConst* மாறிலி constp = VN_CAST(exprp, Const);
            எனில் (!constp) திரும்பு பொய்;
            width += static_cast<uint64_t>(constp->width_சொற்கள்()) * VL_EDATASIZE;
            hasData = உண்மை;
            திரும்பு !constp->இலக்கம்().isFourState();
        });
        எனில் (!emittable) திரும்பு பொய்;
        // Dynamically sized values allocate when built with data, so use the pool for them
        எனில் (hasDynamic) திரும்பு hasData;
        திரும்பு width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_முனை_கோவை* nodep) {
        // If enclosing statement is an assign
        Ast_முனை_Assign* மாறிலி assignp = VN_CAST(m_stmtp, முனை_Assign);
        எனில் (!assignp) திரும்பு nullptr;
        // of which 'nodep' is the RHS of
        எனில் (assignp->rhsp() != nodep) திரும்பு nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* மாறிலி lRefp = VN_CAST(assignp->lhsp(), VarRef);
        எனில் (!lRefp) திரும்பு nullptr;
        AstVar* மாறிலி varp = lRefp->varp();
        // And the RHS does not use the same variable
        எனில் (nodep->exists([varp](மாறிலி AstVarRef* refp) { திரும்பு refp->varp() == varp; })) {
            திரும்பு nullptr;
        }
        // Then return the LHS reference
        திரும்பு lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_முனை_கோவை* nodep) {
        File_வரி* மாறிலி flp = nodep->fileline();
        மாறிலி std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(உண்மை);
        varp->noReset(உண்மை);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        திரும்பு varp;
    }

    AstVar* createTemp(Ast_முனை_கோவை* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_வரி* மாறிலி flp = nodep->fileline();
        AstVar* varp = nullptr;

        எனில் (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* மாறிலி refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } இல்லையெனில் {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* மாறிலி assignp = புதிய AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(புதிய AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        திரும்பு varp;
    }

    வெறுமை replaceWithOvrShift(Ast_முனை_Biop* nodep) {
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* newp = nullptr;
        எனில் (VN_IS(nodep, ShiftL)) {
            newp = புதிய AstShiftLOvr{flp, lhsp, rhsp};
        } இல்லையெனில் எனில் (VN_IS(nodep, ShiftR)) {
            newp = புதிய AstShiftROvr{flp, lhsp, rhsp};
        } இல்லையெனில் {
            newp = புதிய AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visitShift(Ast_முனை_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_வரி* மாறிலி flp = nodep->fileline();
        எனில் (மாறிலி AstConst* மாறிலி shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            மாறிலி தருக்க isOversized
                = shiftp->இலக்கம்().mostSetBitP1() > 32
                  || (shiftp->இலக்கம்().toUInt() >= static_cast<uint32_t>(nodep->width()));
            எனில் (isOversized) {
                // If signed shift, replace with replicated MSB
                எனில் (VN_IS(nodep, ShiftRS)) {
                    Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_முனை_கோவை* மாறிலி msbp = புதிய AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(புதிய AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
                // Unsigned. If pure, replace with zero
                எனில் (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        புதிய AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
                // Impure. Keep shift
                எனில் (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    திரும்பு;
                }
            }
        } இல்லையெனில் {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            எனில் (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                திரும்பு;
            }
        }
        iterateChildren(nodep);
        check_முனை(nodep);
    }

    நிலையான தருக்க rhsReadsLhs(மாறிலி Ast_முனை_Assign* nodep) {
        மாறிலி VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](மாறிலி AstVarRef* refp) {
            எனில் (refp->access().isWriteOrRW()) refp->varp()->user3(உண்மை);
        });
        திரும்பு nodep->rhsp()->exists([](மாறிலி AstVarRef* refp) {
            திரும்பு refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    வெறுமை visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    எனில் (!m_cfuncp) திரும்பு; \
    எனில் ((stmtp)->user1SetOnce()) திரும்பு; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = பொய்; \
    m_stmtp = (stmtp);

    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        எனில் (AstCvtArrayToPacked* மாறிலி packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_முனை_கோவை* மாறிலி கோவை_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(கோவை_Lhsp);
            VL_DO_DANGLING(தள்ளு_Deletep(packedp), packedp);
            Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(புதிய AstCvtPackedToArray{rhsp->fileline(), rhsp, கோவை_Lhsp->dtypep()});
            nodep->dtypeFrom(கோவை_Lhsp);
        }
        // Direct assignment to a simple variable
        எனில் (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_முனை* மாறிலி rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            எனில் (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) திரும்பு;
            எனில் (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } இல்லையெனில் எனில் (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(உண்மை);
            } இல்லையெனில் எனில் (rhsp->width_சொற்கள்() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                திரும்பு;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        மாறிலி தருக்க isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        எனில் (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } இல்லையெனில் {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = உண்மை;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        எனில் (AstVarRef* மாறிலி rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            எனில் (AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                எனில் (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    வெறுமை visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        எனில் (v3Global.opt.autoflush()) {
            மாறிலி Ast_முனை* searchp = nodep->nextp();
            போது (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            எனில் (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_மரம்(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } இல்லையெனில் {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    புதிய AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_மரம்_Pure(உண்மை) : nullptr});
            }
        }
    }
    வெறுமை visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(உண்மை);
        } இல்லையெனில் {
            createTemp(nodep->memp())->noSubst(உண்மை);
        }
    }
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    வெறுமை visit(AstShiftL* nodep) override { visitShift(nodep); }
    வெறுமை visit(AstShiftR* nodep) override { visitShift(nodep); }
    வெறுமை visit(AstShiftRS* nodep) override { visitShift(nodep); }

    வெறுமை visit(AstConst* nodep) override { check_முனை(nodep); }
    // Operators
    வெறுமை visit(Ast_முனை_Termop* nodep) override { check_முனை(nodep); }
    வெறுமை visit(Ast_முனை_Uniop* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
        எனில் (!VN_IS(nodep->backp(), முனை_Assign)) createTemp(nodep);
    }
    வெறுமை visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_முனை(nodep);
    }
    வெறுமை visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = பொய்;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_முனை(nodep);
    }
    வெறுமை visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = பொய்;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    வெறுமை visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = பொய்;
            iterateAndNextNull(nodep->bitp());
        }
        check_முனை(nodep);
    }
    வெறுமை visit(Ast_முனை_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        எனில் (!m_inCons && nodep->is_Constant_மரம்()) {
            எனில் (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            திரும்பு;
        }
        VL_RESTORER(m_inCons);
        m_inCons = உண்மை;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    வெறுமை visit(AstConsAssocItem* nodep) override {
        எனில் (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    வெறுமை visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        எனில் (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* மாறிலி varp = createTemp(nodep);
            varp->noSubst(உண்மை);  // Do not re-inline in V3Subst
        }
    }
    வெறுமை visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        எனில் (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* மாறிலி lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(உண்மை);

            File_வரி* மாறிலி flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* மாறிலி thenRefp = புதிய AstVarRef{flp, varp, VAccess::WRITE};
            எனில் (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_முனை_கோவை* மாறிலி thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* மாறிலி thenAsspp = புதிய AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* மாறிலி elseRefp = புதிய AstVarRef{flp, varp, VAccess::WRITE};
            எனில் (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_முனை_கோவை* மாறிலி elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* மாறிலி elseAsspp = புதிய AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_முனை_கோவை* மாறிலி condp = nodep->condp()->unlinkFrBack();
            AstIf* மாறிலி ifp = புதிய AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_மரம்();
            // Iterate the resulting assignments
            iterate(ifp);
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_முனை_கோவை* exprsp = nodep->exprsp();
        எனில் (nodep->கோவை_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), முனை_கோவை);
            க்கு (Ast_முனை* argp = exprsp; argp; argp = argp->nextp()) {
                மாறிலி AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
                எனில் (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* மாறிலி varp = createTemp(nodep->exprsp());
                    varp->noSubst(உண்மை);
                    நிறுத்து;
                }
            }
        }
        க்கு (Ast_முனை_கோவை *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), முனை_கோவை);

            AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
            Ast_முனை_கோவை* மாறிலி subargp = fargp ? fargp->exprp() : argp;
            எனில் (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* மாறிலி மதிப்பு_Varp = createTemp(subargp);
                மதிப்பு_Varp->noSubst(உண்மை);
                எனில் (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* மாறிலி nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(உண்மை);
                }
                தொடர்;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            எனில் (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* மாறிலி varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(உண்மை);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    வெறுமை visit(AstVar*) override {}  // Don't hit varrefs under vars
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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

வெறுமை V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("premit", 0, dump_மரம்_Either_Level() >= 3);
}
