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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Premit.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr पूर्णांक STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

वर्ग PremitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    स्थिरांक VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    माप_प्रकार m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_गाठ* m_stmtp = nullptr;  // Current statement
    तार्किक m_assignLhs = असत्य;  // Inside assignment lhs, don't breakup extracts
    तार्किक m_inCons = असत्य;  // Inside a NodeCons, so not the root of a value

    // METHODS
    तार्किक needsTemp(Ast_गाठ_अभिव्यक्ती* nodep) {
        // Consider adding a temp for this expression.
        जर (!m_stmtp) परत असत्य;  // Not under a statement
        जर (nodep->user1SetOnce()) परत असत्य;  // Already processed
        जर (!nodep->isWide()) परत असत्य;  // Not wide
        जर (m_assignLhs) परत असत्य;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        परत सत्य;
    }

    शून्य check_गाठ(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    स्थिर तार्किक useConstPool(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
        uint64_t width = 0;
        तार्किक hasData = असत्य;
        तार्किक hasDynamic = असत्य;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        स्थिरांक तार्किक emittable = nodep->forall([&](स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
            जर (VN_IS(exprp, गाठ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                परत सत्य;
            }
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(exprp, Const);
            जर (!constp) परत असत्य;
            width += static_cast<uint64_t>(constp->width_शब्द()) * VL_EDATASIZE;
            hasData = सत्य;
            परत !constp->अंक().isFourState();
        });
        जर (!emittable) परत असत्य;
        // Dynamically sized values allocate when built with data, so use the pool for them
        जर (hasDynamic) परत hasData;
        परत width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_गाठ_अभिव्यक्ती* nodep) {
        // If enclosing statement is an assign
        Ast_गाठ_Assign* स्थिरांक assignp = VN_CAST(m_stmtp, गाठ_Assign);
        जर (!assignp) परत nullptr;
        // of which 'nodep' is the RHS of
        जर (assignp->rhsp() != nodep) परत nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* स्थिरांक lRefp = VN_CAST(assignp->lhsp(), VarRef);
        जर (!lRefp) परत nullptr;
        AstVar* स्थिरांक varp = lRefp->varp();
        // And the RHS does not use the same variable
        जर (nodep->exists([varp](स्थिरांक AstVarRef* refp) { परत refp->varp() == varp; })) {
            परत nullptr;
        }
        // Then return the LHS reference
        परत lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_गाठ_अभिव्यक्ती* nodep) {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        स्थिरांक std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* स्थिरांक varp = नवीन AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(सत्य);
        varp->noReset(सत्य);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        परत varp;
    }

    AstVar* createTemp(Ast_गाठ_अभिव्यक्ती* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstVar* varp = nullptr;

        जर (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* स्थिरांक refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } अन्यथा {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* स्थिरांक assignp = नवीन AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(नवीन AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        परत varp;
    }

    शून्य replaceWithOvrShift(Ast_गाठ_Biop* nodep) {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* newp = nullptr;
        जर (VN_IS(nodep, ShiftL)) {
            newp = नवीन AstShiftLOvr{flp, lhsp, rhsp};
        } अन्यथा जर (VN_IS(nodep, ShiftR)) {
            newp = नवीन AstShiftROvr{flp, lhsp, rhsp};
        } अन्यथा {
            newp = नवीन AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visitShift(Ast_गाठ_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (स्थिरांक AstConst* स्थिरांक shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            स्थिरांक तार्किक isOversized
                = shiftp->अंक().mostSetBitP1() > 32
                  || (shiftp->अंक().toUInt() >= static_cast<uint32_t>(nodep->width()));
            जर (isOversized) {
                // If signed shift, replace with replicated MSB
                जर (VN_IS(nodep, ShiftRS)) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक msbp = नवीन AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(नवीन AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
                // Unsigned. If pure, replace with zero
                जर (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        नवीन AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
                // Impure. Keep shift
                जर (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    परत;
                }
            }
        } अन्यथा {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            जर (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                परत;
            }
        }
        iterateChildren(nodep);
        check_गाठ(nodep);
    }

    स्थिर तार्किक rhsReadsLhs(स्थिरांक Ast_गाठ_Assign* nodep) {
        स्थिरांक VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](स्थिरांक AstVarRef* refp) {
            जर (refp->access().isWriteOrRW()) refp->varp()->user3(सत्य);
        });
        परत nodep->rhsp()->exists([](स्थिरांक AstVarRef* refp) {
            परत refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    जर (!m_cfuncp) परत; \
    जर ((stmtp)->user1SetOnce()) परत; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = असत्य; \
    m_stmtp = (stmtp);

    शून्य visit(Ast_गाठ_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        जर (AstCvtArrayToPacked* स्थिरांक packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक अभिव्यक्ती_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(अभिव्यक्ती_Lhsp);
            VL_DO_DANGLING(ढकला_Deletep(packedp), packedp);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(नवीन AstCvtPackedToArray{rhsp->fileline(), rhsp, अभिव्यक्ती_Lhsp->dtypep()});
            nodep->dtypeFrom(अभिव्यक्ती_Lhsp);
        }
        // Direct assignment to a simple variable
        जर (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_गाठ* स्थिरांक rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            जर (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) परत;
            जर (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } अन्यथा जर (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(सत्य);
            } अन्यथा जर (rhsp->width_शब्द() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                परत;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        स्थिरांक तार्किक isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        जर (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } अन्यथा {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = सत्य;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        जर (AstVarRef* स्थिरांक rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            जर (AstVarRef* स्थिरांक lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                जर (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    शून्य visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        जर (v3Global.opt.autoflush()) {
            स्थिरांक Ast_गाठ* searchp = nodep->nextp();
            पर्यंत (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            जर (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_वृक्ष(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } अन्यथा {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    नवीन AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_वृक्ष_Pure(सत्य) : nullptr});
            }
        }
    }
    शून्य visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        जर (स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(सत्य);
        } अन्यथा {
            createTemp(nodep->memp())->noSubst(सत्य);
        }
    }
    शून्य visit(Ast_गाठ_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    शून्य visit(AstShiftL* nodep) override { visitShift(nodep); }
    शून्य visit(AstShiftR* nodep) override { visitShift(nodep); }
    शून्य visit(AstShiftRS* nodep) override { visitShift(nodep); }

    शून्य visit(AstConst* nodep) override { check_गाठ(nodep); }
    // Operators
    शून्य visit(Ast_गाठ_Termop* nodep) override { check_गाठ(nodep); }
    शून्य visit(Ast_गाठ_Uniop* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(Ast_गाठ_Biop* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
        जर (!VN_IS(nodep->backp(), गाठ_Assign)) createTemp(nodep);
    }
    शून्य visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_गाठ(nodep);
    }
    शून्य visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = असत्य;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_गाठ(nodep);
    }
    शून्य visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = असत्य;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    शून्य visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = असत्य;
            iterateAndNextNull(nodep->bitp());
        }
        check_गाठ(nodep);
    }
    शून्य visit(Ast_गाठ_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        जर (!m_inCons && nodep->is_Constant_वृक्ष()) {
            जर (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            परत;
        }
        VL_RESTORER(m_inCons);
        m_inCons = सत्य;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    शून्य visit(AstConsAssocItem* nodep) override {
        जर (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    शून्य visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        जर (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* स्थिरांक varp = createTemp(nodep);
            varp->noSubst(सत्य);  // Do not re-inline in V3Subst
        }
    }
    शून्य visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        जर (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* स्थिरांक lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(सत्य);

            File_ओळ* स्थिरांक flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* स्थिरांक thenRefp = नवीन AstVarRef{flp, varp, VAccess::WRITE};
            जर (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* स्थिरांक thenAsspp = नवीन AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* स्थिरांक elseRefp = नवीन AstVarRef{flp, varp, VAccess::WRITE};
            जर (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* स्थिरांक elseAsspp = नवीन AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = nodep->condp()->unlinkFrBack();
            AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_वृक्ष();
            // Iterate the resulting assignments
            iterate(ifp);
            परत;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_गाठ_अभिव्यक्ती* exprsp = nodep->exprsp();
        जर (nodep->अभिव्यक्ती_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), गाठ_अभिव्यक्ती);
            साठी (Ast_गाठ* argp = exprsp; argp; argp = argp->nextp()) {
                स्थिरांक AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
                जर (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* स्थिरांक varp = createTemp(nodep->exprsp());
                    varp->noSubst(सत्य);
                    थांबा;
                }
            }
        }
        साठी (Ast_गाठ_अभिव्यक्ती *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), गाठ_अभिव्यक्ती);

            AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक subargp = fargp ? fargp->exprp() : argp;
            जर (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* स्थिरांक मूल्य_Varp = createTemp(subargp);
                मूल्य_Varp->noSubst(सत्य);
                जर (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* स्थिरांक nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(सत्य);
                }
                सुरूठेवा;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            जर (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* स्थिरांक varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(सत्य);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    शून्य visit(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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

शून्य V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("premit", 0, dump_वृक्ष_Either_Level() >= 3);
}
