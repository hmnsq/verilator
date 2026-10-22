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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Premit.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr പൂർണ്ണസംഖ്യ STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

വർഗ്ഗം PremitVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_കണ്ണി* m_stmtp = nullptr;  // Current statement
    തർക്കം m_assignLhs = അസത്യം;  // Inside assignment lhs, don't breakup extracts
    തർക്കം m_inCons = അസത്യം;  // Inside a NodeCons, so not the root of a value

    // METHODS
    തർക്കം needsTemp(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // Consider adding a temp for this expression.
        എങ്കിൽ (!m_stmtp) തിരികെ അസത്യം;  // Not under a statement
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ അസത്യം;  // Already processed
        എങ്കിൽ (!nodep->isWide()) തിരികെ അസത്യം;  // Not wide
        എങ്കിൽ (m_assignLhs) തിരികെ അസത്യം;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        തിരികെ സത്യം;
    }

    ശൂന്യം check_കണ്ണി(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    സ്ഥിതം തർക്കം useConstPool(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        uint64_t width = 0;
        തർക്കം hasData = അസത്യം;
        തർക്കം hasDynamic = അസത്യം;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        സ്ഥിരം തർക്കം emittable = nodep->forall([&](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
            എങ്കിൽ (VN_IS(exprp, കണ്ണി_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                തിരികെ സത്യം;
            }
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(exprp, Const);
            എങ്കിൽ (!constp) തിരികെ അസത്യം;
            width += static_cast<uint64_t>(constp->width_വാക്കുകൾ()) * VL_EDATASIZE;
            hasData = സത്യം;
            തിരികെ !constp->അക്കം().isFourState();
        });
        എങ്കിൽ (!emittable) തിരികെ അസത്യം;
        // Dynamically sized values allocate when built with data, so use the pool for them
        എങ്കിൽ (hasDynamic) തിരികെ hasData;
        തിരികെ width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // If enclosing statement is an assign
        Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_CAST(m_stmtp, കണ്ണി_Assign);
        എങ്കിൽ (!assignp) തിരികെ nullptr;
        // of which 'nodep' is the RHS of
        എങ്കിൽ (assignp->rhsp() != nodep) തിരികെ nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* സ്ഥിരം lRefp = VN_CAST(assignp->lhsp(), VarRef);
        എങ്കിൽ (!lRefp) തിരികെ nullptr;
        AstVar* സ്ഥിരം varp = lRefp->varp();
        // And the RHS does not use the same variable
        എങ്കിൽ (nodep->exists([varp](സ്ഥിരം AstVarRef* refp) { തിരികെ refp->varp() == varp; })) {
            തിരികെ nullptr;
        }
        // Then return the LHS reference
        തിരികെ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        സ്ഥിരം std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(സത്യം);
        varp->noReset(സത്യം);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        തിരികെ varp;
    }

    AstVar* createTemp(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstVar* varp = nullptr;

        എങ്കിൽ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* സ്ഥിരം refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } അല്ലെങ്കിൽ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        തിരികെ varp;
    }

    ശൂന്യം replaceWithOvrShift(Ast_കണ്ണി_Biop* nodep) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
        എങ്കിൽ (VN_IS(nodep, ShiftL)) {
            newp = പുതിയ AstShiftLOvr{flp, lhsp, rhsp};
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, ShiftR)) {
            newp = പുതിയ AstShiftROvr{flp, lhsp, rhsp};
        } അല്ലെങ്കിൽ {
            newp = പുതിയ AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visitShift(Ast_കണ്ണി_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            സ്ഥിരം തർക്കം isOversized
                = shiftp->അക്കം().mostSetBitP1() > 32
                  || (shiftp->അക്കം().toUInt() >= static_cast<uint32_t>(nodep->width()));
            എങ്കിൽ (isOversized) {
                // If signed shift, replace with replicated MSB
                എങ്കിൽ (VN_IS(nodep, ShiftRS)) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം msbp = പുതിയ AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(പുതിയ AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
                // Unsigned. If pure, replace with zero
                എങ്കിൽ (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        പുതിയ AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
                // Impure. Keep shift
                എങ്കിൽ (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    തിരികെ;
                }
            }
        } അല്ലെങ്കിൽ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            എങ്കിൽ (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                തിരികെ;
            }
        }
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }

    സ്ഥിതം തർക്കം rhsReadsLhs(സ്ഥിരം Ast_കണ്ണി_Assign* nodep) {
        സ്ഥിരം VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](സ്ഥിരം AstVarRef* refp) {
            എങ്കിൽ (refp->access().isWriteOrRW()) refp->varp()->user3(സത്യം);
        });
        തിരികെ nodep->rhsp()->exists([](സ്ഥിരം AstVarRef* refp) {
            തിരികെ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    ശൂന്യം visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    എങ്കിൽ (!m_cfuncp) തിരികെ; \
    എങ്കിൽ ((stmtp)->user1SetOnce()) തിരികെ; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = അസത്യം; \
    m_stmtp = (stmtp);

    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        എങ്കിൽ (AstCvtArrayToPacked* സ്ഥിരം packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം വ്യഞ്ജകം_Lhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(വ്യഞ്ജകം_Lhsp);
            VL_DO_DANGLING(തള്ളുക_Deletep(packedp), packedp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(പുതിയ AstCvtPackedToArray{rhsp->fileline(), rhsp, വ്യഞ്ജകം_Lhsp->dtypep()});
            nodep->dtypeFrom(വ്യഞ്ജകം_Lhsp);
        }
        // Direct assignment to a simple variable
        എങ്കിൽ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_കണ്ണി* സ്ഥിരം rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            എങ്കിൽ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) തിരികെ;
            എങ്കിൽ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (rhsp->width_വാക്കുകൾ() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                തിരികെ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        സ്ഥിരം തർക്കം isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        എങ്കിൽ (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } അല്ലെങ്കിൽ {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = സത്യം;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        എങ്കിൽ (AstVarRef* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                എങ്കിൽ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    ശൂന്യം visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (v3Global.opt.autoflush()) {
            സ്ഥിരം Ast_കണ്ണി* searchp = nodep->nextp();
            ആകുമ്പോൾ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            എങ്കിൽ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_വൃക്ഷം(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } അല്ലെങ്കിൽ {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    പുതിയ AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_വൃക്ഷം_Pure(സത്യം) : nullptr});
            }
        }
    }
    ശൂന്യം visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(സത്യം);
        } അല്ലെങ്കിൽ {
            createTemp(nodep->memp())->noSubst(സത്യം);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    ശൂന്യം visit(AstShiftL* nodep) override { visitShift(nodep); }
    ശൂന്യം visit(AstShiftR* nodep) override { visitShift(nodep); }
    ശൂന്യം visit(AstShiftRS* nodep) override { visitShift(nodep); }

    ശൂന്യം visit(AstConst* nodep) override { check_കണ്ണി(nodep); }
    // Operators
    ശൂന്യം visit(Ast_കണ്ണി_Termop* nodep) override { check_കണ്ണി(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstRand* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
        എങ്കിൽ (!VN_IS(nodep->backp(), കണ്ണി_Assign)) createTemp(nodep);
    }
    ശൂന്യം visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = അസത്യം;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = അസത്യം;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    ശൂന്യം visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = അസത്യം;
            iterateAndNextNull(nodep->bitp());
        }
        check_കണ്ണി(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        എങ്കിൽ (!m_inCons && nodep->is_Constant_വൃക്ഷം()) {
            എങ്കിൽ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            തിരികെ;
        }
        VL_RESTORER(m_inCons);
        m_inCons = സത്യം;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    ശൂന്യം visit(AstConsAssocItem* nodep) override {
        എങ്കിൽ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    ശൂന്യം visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* സ്ഥിരം varp = createTemp(nodep);
            varp->noSubst(സത്യം);  // Do not re-inline in V3Subst
        }
    }
    ശൂന്യം visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        എങ്കിൽ (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* സ്ഥിരം lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(സത്യം);

            File_വരി* സ്ഥിരം flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* സ്ഥിരം thenRefp = പുതിയ AstVarRef{flp, varp, VAccess::WRITE};
            എങ്കിൽ (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* സ്ഥിരം thenAsspp = പുതിയ AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* സ്ഥിരം elseRefp = പുതിയ AstVarRef{flp, varp, VAccess::WRITE};
            എങ്കിൽ (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* സ്ഥിരം elseAsspp = പുതിയ AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = nodep->condp()->unlinkFrBack();
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_വൃക്ഷം();
            // Iterate the resulting assignments
            iterate(ifp);
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_കണ്ണി_വ്യഞ്ജകം* exprsp = nodep->exprsp();
        എങ്കിൽ (nodep->വ്യഞ്ജകം_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), കണ്ണി_വ്യഞ്ജകം);
            വേണ്ടി (Ast_കണ്ണി* argp = exprsp; argp; argp = argp->nextp()) {
                സ്ഥിരം AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
                എങ്കിൽ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* സ്ഥിരം varp = createTemp(nodep->exprsp());
                    varp->noSubst(സത്യം);
                    നിർത്തുക;
                }
            }
        }
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), കണ്ണി_വ്യഞ്ജകം);

            AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം subargp = fargp ? fargp->exprp() : argp;
            എങ്കിൽ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* സ്ഥിരം മൂല്യം_Varp = createTemp(subargp);
                മൂല്യം_Varp->noSubst(സത്യം);
                എങ്കിൽ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* സ്ഥിരം nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(സത്യം);
                }
                തുടരുക;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            എങ്കിൽ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* സ്ഥിരം varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(സത്യം);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    ശൂന്യം visit(AstVar*) override {}  // Don't hit varrefs under vars
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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

ശൂന്യം V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("premit", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
