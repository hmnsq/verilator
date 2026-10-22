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

constexpr पूर्णाङ्क STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

वर्ग PremitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ध्रुव VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_गाठ* m_stmtp = nullptr;  // Current statement
    तार्किक m_assignLhs = असत्य;  // Inside assignment lhs, don't breakup extracts
    तार्किक m_inCons = असत्य;  // Inside a NodeCons, so not the root of a value

    // METHODS
    तार्किक needsTemp(Ast_गाठ_अभिव्यक्ती* nodep) {
        // Consider adding a temp for this expression.
        यदि (!m_stmtp) प्रत्यागम असत्य;  // Not under a statement
        यदि (nodep->user1SetOnce()) प्रत्यागम असत्य;  // Already processed
        यदि (!nodep->isWide()) प्रत्यागम असत्य;  // Not wide
        यदि (m_assignLhs) प्रत्यागम असत्य;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        प्रत्यागम सत्य;
    }

    शून्य check_गाठ(Ast_गाठ_अभिव्यक्ती* nodep) {
        यदि (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    स्थिर तार्किक useConstPool(ध्रुव Ast_गाठ_अभिव्यक्ती* nodep) {
        uint64_t width = 0;
        तार्किक hasData = असत्य;
        तार्किक hasDynamic = असत्य;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ध्रुव तार्किक emittable = nodep->forall([&](ध्रुव Ast_गाठ_अभिव्यक्ती* exprp) {
            यदि (VN_IS(exprp, गाठ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                प्रत्यागम सत्य;
            }
            ध्रुव AstConst* ध्रुव constp = VN_CAST(exprp, Const);
            यदि (!constp) प्रत्यागम असत्य;
            width += static_cast<uint64_t>(constp->width_शब्द()) * VL_EDATASIZE;
            hasData = सत्य;
            प्रत्यागम !constp->अंक().isFourState();
        });
        यदि (!emittable) प्रत्यागम असत्य;
        // Dynamically sized values allocate when built with data, so use the pool for them
        यदि (hasDynamic) प्रत्यागम hasData;
        प्रत्यागम width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_गाठ_अभिव्यक्ती* nodep) {
        // If enclosing statement is an assign
        Ast_गाठ_Assign* ध्रुव assignp = VN_CAST(m_stmtp, गाठ_Assign);
        यदि (!assignp) प्रत्यागम nullptr;
        // of which 'nodep' is the RHS of
        यदि (assignp->rhsp() != nodep) प्रत्यागम nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ध्रुव lRefp = VN_CAST(assignp->lhsp(), VarRef);
        यदि (!lRefp) प्रत्यागम nullptr;
        AstVar* ध्रुव varp = lRefp->varp();
        // And the RHS does not use the same variable
        यदि (nodep->exists([varp](ध्रुव AstVarRef* refp) { प्रत्यागम refp->varp() == varp; })) {
            प्रत्यागम nullptr;
        }
        // Then return the LHS reference
        प्रत्यागम lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_गाठ_अभिव्यक्ती* nodep) {
        File_ओळ* ध्रुव flp = nodep->fileline();
        ध्रुव std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(सत्य);
        varp->noReset(सत्य);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        प्रत्यागम varp;
    }

    AstVar* createTemp(Ast_गाठ_अभिव्यक्ती* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_ओळ* ध्रुव flp = nodep->fileline();
        AstVar* varp = nullptr;

        यदि (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ध्रुव refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } अन्यथा {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* ध्रुव assignp = नूतन AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(नूतन AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        प्रत्यागम varp;
    }

    शून्य replaceWithOvrShift(Ast_गाठ_Biop* nodep) {
        File_ओळ* ध्रुव flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* newp = nullptr;
        यदि (VN_IS(nodep, ShiftL)) {
            newp = नूतन AstShiftLOvr{flp, lhsp, rhsp};
        } अन्यथा यदि (VN_IS(nodep, ShiftR)) {
            newp = नूतन AstShiftROvr{flp, lhsp, rhsp};
        } अन्यथा {
            newp = नूतन AstShiftRSOvr{flp, lhsp, rhsp};
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
        File_ओळ* ध्रुव flp = nodep->fileline();
        यदि (ध्रुव AstConst* ध्रुव shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ध्रुव तार्किक isOversized
                = shiftp->अंक().mostSetBitP1() > 32
                  || (shiftp->अंक().toUInt() >= static_cast<uint32_t>(nodep->width()));
            यदि (isOversized) {
                // If signed shift, replace with replicated MSB
                यदि (VN_IS(nodep, ShiftRS)) {
                    Ast_गाठ_अभिव्यक्ती* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_गाठ_अभिव्यक्ती* ध्रुव msbp = नूतन AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(नूतन AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                // Unsigned. If pure, replace with zero
                यदि (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        नूतन AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                // Impure. Keep shift
                यदि (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    प्रत्यागम;
                }
            }
        } अन्यथा {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            यदि (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                प्रत्यागम;
            }
        }
        iterateChildren(nodep);
        check_गाठ(nodep);
    }

    स्थिर तार्किक rhsReadsLhs(ध्रुव Ast_गाठ_Assign* nodep) {
        ध्रुव VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ध्रुव AstVarRef* refp) {
            यदि (refp->access().isWriteOrRW()) refp->varp()->user3(सत्य);
        });
        प्रत्यागम nodep->rhsp()->exists([](ध्रुव AstVarRef* refp) {
            प्रत्यागम refp->access().isReadOnly() && refp->varp()->user3();
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
    यदि (!m_cfuncp) प्रत्यागम; \
    यदि ((stmtp)->user1SetOnce()) प्रत्यागम; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = असत्य; \
    m_stmtp = (stmtp);

    शून्य visit(Ast_गाठ_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        यदि (AstCvtArrayToPacked* ध्रुव packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_गाठ_अभिव्यक्ती* ध्रुव अभिव्यक्ती_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(अभिव्यक्ती_Lhsp);
            VL_DO_DANGLING(ढकला_Deletep(packedp), packedp);
            Ast_गाठ_अभिव्यक्ती* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(नूतन AstCvtPackedToArray{rhsp->fileline(), rhsp, अभिव्यक्ती_Lhsp->dtypep()});
            nodep->dtypeFrom(अभिव्यक्ती_Lhsp);
        }
        // Direct assignment to a simple variable
        यदि (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_गाठ* ध्रुव rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            यदि (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) प्रत्यागम;
            यदि (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } अन्यथा यदि (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(सत्य);
            } अन्यथा यदि (rhsp->width_शब्द() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                प्रत्यागम;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ध्रुव तार्किक isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        यदि (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } अन्यथा {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = सत्य;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        यदि (AstVarRef* ध्रुव rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            यदि (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                यदि (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    शून्य visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        यदि (v3Global.opt.autoflush()) {
            ध्रुव Ast_गाठ* searchp = nodep->nextp();
            यावत् (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            यदि (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_वृक्ष(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } अन्यथा {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    नूतन AstFFlush{nodep->fileline(),
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
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep->memp(), VarRef)) {
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
        यदि (!VN_IS(nodep->backp(), गाठ_Assign)) createTemp(nodep);
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
        यदि (!m_inCons && nodep->is_Constant_वृक्ष()) {
            यदि (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            प्रत्यागम;
        }
        VL_RESTORER(m_inCons);
        m_inCons = सत्य;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    शून्य visit(AstConsAssocItem* nodep) override {
        यदि (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    शून्य visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ध्रुव varp = createTemp(nodep);
            varp->noSubst(सत्य);  // Do not re-inline in V3Subst
        }
    }
    शून्य visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        यदि (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ध्रुव lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(सत्य);

            File_ओळ* ध्रुव flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ध्रुव thenRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            यदि (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_गाठ_अभिव्यक्ती* ध्रुव thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ध्रुव thenAsspp = नूतन AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ध्रुव elseRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            यदि (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_गाठ_अभिव्यक्ती* ध्रुव elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ध्रुव elseAsspp = नूतन AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_गाठ_अभिव्यक्ती* ध्रुव condp = nodep->condp()->unlinkFrBack();
            AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_वृक्ष();
            // Iterate the resulting assignments
            iterate(ifp);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_गाठ_अभिव्यक्ती* exprsp = nodep->exprsp();
        यदि (nodep->अभिव्यक्ती_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), गाठ_अभिव्यक्ती);
            प्रति (Ast_गाठ* argp = exprsp; argp; argp = argp->nextp()) {
                ध्रुव AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
                यदि (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ध्रुव varp = createTemp(nodep->exprsp());
                    varp->noSubst(सत्य);
                    भङ्ग;
                }
            }
        }
        प्रति (Ast_गाठ_अभिव्यक्ती *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), गाठ_अभिव्यक्ती);

            AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
            Ast_गाठ_अभिव्यक्ती* ध्रुव subargp = fargp ? fargp->exprp() : argp;
            यदि (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ध्रुव मूल्य_Varp = createTemp(subargp);
                मूल्य_Varp->noSubst(सत्य);
                यदि (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ध्रुव nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(सत्य);
                }
                अनुवर्त;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            यदि (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ध्रुव varp = createTemp(subargp);
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
