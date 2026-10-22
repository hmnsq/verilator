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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Premit.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

ଶ୍ରେଣୀ PremitVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ଗଣ୍ଠି* m_stmtp = nullptr;  // Current statement
    ତାର୍କିକ m_assignLhs = ମିଥ୍ୟା;  // Inside assignment lhs, don't breakup extracts
    ତାର୍କିକ m_inCons = ମିଥ୍ୟା;  // Inside a NodeCons, so not the root of a value

    // METHODS
    ତାର୍କିକ needsTemp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // Consider adding a temp for this expression.
        ଯଦି (!m_stmtp) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Not under a statement
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Already processed
        ଯଦି (!nodep->isWide()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Not wide
        ଯଦି (m_assignLhs) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ check_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ସ୍ଥିର ତାର୍କିକ useConstPool(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        uint64_t width = 0;
        ତାର୍କିକ hasData = ମିଥ୍ୟା;
        ତାର୍କିକ hasDynamic = ମିଥ୍ୟା;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ଅଚଳ ତାର୍କିକ emittable = nodep->forall([&](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
            ଯଦି (VN_IS(exprp, ଗଣ୍ଠି_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(exprp, Const);
            ଯଦି (!constp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            width += static_cast<uint64_t>(constp->width_ଶବ୍ଦଗୁଡ଼ିକ()) * VL_EDATASIZE;
            hasData = ସତ୍ୟ;
            ଫେରନ୍ତୁ !constp->ଅଙ୍କ().isFourState();
        });
        ଯଦି (!emittable) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ଯଦି (hasDynamic) ଫେରନ୍ତୁ hasData;
        ଫେରନ୍ତୁ width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        // If enclosing statement is an assign
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_CAST(m_stmtp, ଗଣ୍ଠି_Assign);
        ଯଦି (!assignp) ଫେରନ୍ତୁ nullptr;
        // of which 'nodep' is the RHS of
        ଯଦି (assignp->rhsp() != nodep) ଫେରନ୍ତୁ nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* ଅଚଳ lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ଯଦି (!lRefp) ଫେରନ୍ତୁ nullptr;
        AstVar* ଅଚଳ varp = lRefp->varp();
        // And the RHS does not use the same variable
        ଯଦି (nodep->exists([varp](ଅଚଳ AstVarRef* refp) { ଫେରନ୍ତୁ refp->varp() == varp; })) {
            ଫେରନ୍ତୁ nullptr;
        }
        // Then return the LHS reference
        ଫେରନ୍ତୁ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଅଚଳ std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(ସତ୍ୟ);
        varp->noReset(ସତ୍ୟ);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        ଫେରନ୍ତୁ varp;
    }

    AstVar* createTemp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstVar* varp = nullptr;

        ଯଦି (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ଅଚଳ refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ଅନ୍ୟଥା {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ଫେରନ୍ତୁ varp;
    }

    ଶୂନ୍ୟ replaceWithOvrShift(Ast_ଗଣ୍ଠି_Biop* nodep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
        ଯଦି (VN_IS(nodep, ShiftL)) {
            newp = ନୂତନ AstShiftLOvr{flp, lhsp, rhsp};
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, ShiftR)) {
            newp = ନୂତନ AstShiftROvr{flp, lhsp, rhsp};
        } ଅନ୍ୟଥା {
            newp = ନୂତନ AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visitShift(Ast_ଗଣ୍ଠି_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ଅଚଳ ତାର୍କିକ isOversized
                = shiftp->ଅଙ୍କ().mostSetBitP1() > 32
                  || (shiftp->ଅଙ୍କ().toUInt() >= static_cast<uint32_t>(nodep->width()));
            ଯଦି (isOversized) {
                // If signed shift, replace with replicated MSB
                ଯଦି (VN_IS(nodep, ShiftRS)) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ msbp = ନୂତନ AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(ନୂତନ AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
                // Unsigned. If pure, replace with zero
                ଯଦି (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        ନୂତନ AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
                // Impure. Keep shift
                ଯଦି (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    ଫେରନ୍ତୁ;
                }
            }
        } ଅନ୍ୟଥା {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ଯଦି (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                ଫେରନ୍ତୁ;
            }
        }
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }

    ସ୍ଥିର ତାର୍କିକ rhsReadsLhs(ଅଚଳ Ast_ଗଣ୍ଠି_Assign* nodep) {
        ଅଚଳ VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](ଅଚଳ AstVarRef* refp) {
            ଯଦି (refp->access().isWriteOrRW()) refp->varp()->user3(ସତ୍ୟ);
        });
        ଫେରନ୍ତୁ nodep->rhsp()->exists([](ଅଚଳ AstVarRef* refp) {
            ଫେରନ୍ତୁ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    ଯଦି (!m_cfuncp) ଫେରନ୍ତୁ; \
    ଯଦି ((stmtp)->user1SetOnce()) ଫେରନ୍ତୁ; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = ମିଥ୍ୟା; \
    m_stmtp = (stmtp);

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        ଯଦି (AstCvtArrayToPacked* ଅଚଳ packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ପଦସମଷ୍ଟି_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(ପଦସମଷ୍ଟି_Lhsp);
            VL_DO_DANGLING(ଠେଲ_Deletep(packedp), packedp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(ନୂତନ AstCvtPackedToArray{rhsp->fileline(), rhsp, ପଦସମଷ୍ଟି_Lhsp->dtypep()});
            nodep->dtypeFrom(ପଦସମଷ୍ଟି_Lhsp);
        }
        // Direct assignment to a simple variable
        ଯଦି (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ଗଣ୍ଠି* ଅଚଳ rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ଯଦି (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ଫେରନ୍ତୁ;
            ଯଦି (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ଅନ୍ୟଥା ଯଦି (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (rhsp->width_ଶବ୍ଦଗୁଡ଼ିକ() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ଫେରନ୍ତୁ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ଅଚଳ ତାର୍କିକ isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ଯଦି (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } ଅନ୍ୟଥା {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = ସତ୍ୟ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ଯଦି (AstVarRef* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ଯଦି (AstVarRef* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ଯଦି (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ଯଦି (v3Global.opt.autoflush()) {
            ଅଚଳ Ast_ଗଣ୍ଠି* searchp = nodep->nextp();
            ଯେତେବେଳେ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ଯଦି (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_ବୃକ୍ଷ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ଅନ୍ୟଥା {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    ନୂତନ AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ) : nullptr});
            }
        }
    }
    ଶୂନ୍ୟ visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            createTemp(nodep->memp())->noSubst(ସତ୍ୟ);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    ଶୂନ୍ୟ visit(AstShiftL* nodep) override { visitShift(nodep); }
    ଶୂନ୍ୟ visit(AstShiftR* nodep) override { visitShift(nodep); }
    ଶୂନ୍ୟ visit(AstShiftRS* nodep) override { visitShift(nodep); }

    ଶୂନ୍ୟ visit(AstConst* nodep) override { check_ଗଣ୍ଠି(nodep); }
    // Operators
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Termop* nodep) override { check_ଗଣ୍ଠି(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
        ଯଦି (!VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign)) createTemp(nodep);
    }
    ଶୂନ୍ୟ visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ମିଥ୍ୟା;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ମିଥ୍ୟା;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    ଶୂନ୍ୟ visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = ମିଥ୍ୟା;
            iterateAndNextNull(nodep->bitp());
        }
        check_ଗଣ୍ଠି(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ଯଦି (!m_inCons && nodep->is_Constant_ବୃକ୍ଷ()) {
            ଯଦି (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            ଫେରନ୍ତୁ;
        }
        VL_RESTORER(m_inCons);
        m_inCons = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    ଶୂନ୍ୟ visit(AstConsAssocItem* nodep) override {
        ଯଦି (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    ଶୂନ୍ୟ visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ଯଦି (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* ଅଚଳ varp = createTemp(nodep);
            varp->noSubst(ସତ୍ୟ);  // Do not re-inline in V3Subst
        }
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ଯଦି (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ଅଚଳ lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(ସତ୍ୟ);

            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ଅଚଳ thenRefp = ନୂତନ AstVarRef{flp, varp, VAccess::WRITE};
            ଯଦି (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* ଅଚଳ thenAsspp = ନୂତନ AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ଅଚଳ elseRefp = ନୂତନ AstVarRef{flp, varp, VAccess::WRITE};
            ଯଦି (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* ଅଚଳ elseAsspp = ନୂତନ AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = nodep->condp()->unlinkFrBack();
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_ବୃକ୍ଷ();
            // Iterate the resulting assignments
            iterate(ifp);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp = nodep->exprsp();
        ଯଦି (nodep->ପଦସମଷ୍ଟି_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            ପାଇଁ (Ast_ଗଣ୍ଠି* argp = exprsp; argp; argp = argp->nextp()) {
                ଅଚଳ AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
                ଯଦି (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ଅଚଳ varp = createTemp(nodep->exprsp());
                    varp->noSubst(ସତ୍ୟ);
                    ଭାଙ୍ଗ;
                }
            }
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);

            AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ subargp = fargp ? fargp->exprp() : argp;
            ଯଦି (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ଅଚଳ ମୂଲ୍ୟ_Varp = createTemp(subargp);
                ମୂଲ୍ୟ_Varp->noSubst(ସତ୍ୟ);
                ଯଦି (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ଅଚଳ nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(ସତ୍ୟ);
                }
                ଜାରି;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ଯଦି (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ଅଚଳ varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(ସତ୍ୟ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Don't hit varrefs under vars
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
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

ଶୂନ୍ୟ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("premit", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
