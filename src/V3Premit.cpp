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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Premit.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ਪੂਰਨ STATIC_CONST_MIN_ਚੌੜਾਈ = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

ਵਰਗ PremitVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_ਮੰਜ਼ਿਲ_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current block
    ਮਾਪ_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ਗੰਢ* m_stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Current statement
    ਤਾਰਕਿਕ m_ਸੌਂਪੋ_Lhs = ਝੂਠ;  // Inside assignment lhs, don't breakup extracts
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Cons = ਝੂਠ;  // Inside a NodeCons, so not the root of a value

    // METHODS
    ਤਾਰਕਿਕ needs_ਆਰਜ਼ੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // Consider adding a temp for this expression.
        ਜੇ (!m_stmtp) ਵਾਪਸ ਝੂਠ;  // Not under a statement
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ ਝੂਠ;  // Already processed
        ਜੇ (!nodep->isWide()) ਵਾਪਸ ਝੂਠ;  // Not wide
        ਜੇ (m_ਸੌਂਪੋ_Lhs) ਵਾਪਸ ਝੂਠ;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->ਪਹਿਲਾ_Abovep(), ArraySel), nodep, "Should have been ignored");
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ check_ਗੰਢ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (needs_ਆਰਜ਼ੀ(nodep)) create_ਆਰਜ਼ੀ(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ਸਥਿਰ ਤਾਰਕਿਕ useConstPool(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        uint64_t ਚੌੜਾਈ = 0;
        ਤਾਰਕਿਕ has_ਅੰਕੜੇ = ਝੂਠ;
        ਤਾਰਕਿਕ hasDynamic = ਝੂਠ;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ਅਚਲ ਤਾਰਕਿਕ emittable = nodep->forall([&](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
            ਜੇ (VN_IS(exprp, ਗੰਢ_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ਵਾਪਸ ਸੱਚ;
            }
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(exprp, Const);
            ਜੇ (!constp) ਵਾਪਸ ਝੂਠ;
            ਚੌੜਾਈ += static_cast<uint64_t>(constp->ਚੌੜਾਈ_ਸ਼ਬਦ()) * VL_EDATASIZE;
            has_ਅੰਕੜੇ = ਸੱਚ;
            ਵਾਪਸ !constp->ਅੰਕ().isFourState();
        });
        ਜੇ (!emittable) ਵਾਪਸ ਝੂਠ;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ਜੇ (hasDynamic) ਵਾਪਸ has_ਅੰਕੜੇ;
        ਵਾਪਸ ਚੌੜਾਈ >= static_cast<uint64_t>(STATIC_CONST_MIN_ਚੌੜਾਈ);
    }

    AstVarRef* is_Rhs_ਦਾ_ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_Var(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // If enclosing statement is an assign
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(m_stmtp, ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!assignp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // of which 'nodep' is the RHS of
        ਜੇ (assignp->rhsp() != nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // and the LHS is a sipmple variable reference
        AstVarRef* ਅਚਲ lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ਜੇ (!lRefp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVar* ਅਚਲ varp = lRefp->varp();
        // And the RHS does not use the same variable
        ਜੇ (nodep->exists([varp](ਅਚਲ AstVarRef* refp) { ਵਾਪਸ refp->varp() == varp; })) {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // Then return the LHS reference
        ਵਾਪਸ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਅਚਲ std::string name = "__Vtemp_" + std::ਮੰਜ਼ਿਲ_string(++m_tmpVarCnt);
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(ਸੱਚ);
        varp->noReset(ਸੱਚ);
        m_cfuncp->ਜੋੜੋ_Varsp(varp);
        ++m_temporaryVarsCreated;
        ਵਾਪਸ varp;
    }

    AstVar* create_ਆਰਜ਼ੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_ਪਿੱਠ(&relinker);

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstVar* varp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ਅਚਲ refp = V3ConstPool::ਲੱਭੋ(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ++m_extracted_ਮੰਜ਼ਿਲ_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ਨਹੀਂਤਾਂ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE};
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ਵਾਪਸ varp;
    }

    ਖਾਲੀ replace_With_Ovr_ਖਿਸਕਾਓ(Ast_ਗੰਢ_Biop* nodep) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_IS(nodep, ਖਿਸਕਾਓ_L)) {
            newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_LOvr{flp, lhsp, rhsp};
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, ਖਿਸਕਾਓ_R)) {
            newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_ROvr{flp, lhsp, rhsp};
        } ਨਹੀਂਤਾਂ {
            newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ_ਖਿਸਕਾਓ(Ast_ਗੰਢ_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ਖਿਸਕਾਓ_L) || VN_IS(nodep, ਖਿਸਕਾਓ_R) || VN_IS(nodep, ਖਿਸਕਾਓ_RS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ਅਚਲ ਤਾਰਕਿਕ isOversized
                = shiftp->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 32
                  || (shiftp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt() >= static_cast<uint32_t>(nodep->ਚੌੜਾਈ()));
            ਜੇ (isOversized) {
                // If signed shift, replace with replicated MSB
                ਜੇ (VN_IS(nodep, ਖਿਸਕਾਓ_RS)) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ msbp = ਨਵਾਂ AstSel{flp, lhsp, lhsp->ਚੌੜਾਈ_Min() - 1, 1};
                    nodep->replaceWithKeepDType(ਨਵਾਂ AstExtendS{flp, msbp, nodep->ਚੌੜਾਈ()});
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
                // Unsigned. If pure, replace with zero
                ਜੇ (nodep->lhsp()->is_ਸ਼ੁੱਧ()) {
                    nodep->replaceWithKeepDType(
                        ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
                // Impure. Keep shift
                ਜੇ (!nodep->isWide()) {
                    replace_With_Ovr_ਖਿਸਕਾਓ(nodep);
                    ਵਾਪਸ;
                }
            }
        } ਨਹੀਂਤਾਂ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ਜੇ (!nodep->isWide() && (nodep->ਚੌੜਾਈ() < (1LL << nodep->rhsp()->ਚੌੜਾਈ_Min()))) {
                replace_With_Ovr_ਖਿਸਕਾਓ(nodep);
                ਵਾਪਸ;
            }
        }
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }

    ਸਥਿਰ ਤਾਰਕਿਕ rhsReadsLhs(ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
        nodep->lhsp()->foreach([](ਅਚਲ AstVarRef* refp) {
            ਜੇ (refp->access().isWriteOrRW()) refp->varp()->user3(ਸੱਚ);
        });
        ਵਾਪਸ nodep->rhsp()->exists([](ਅਚਲ AstVarRef* refp) {
            ਵਾਪਸ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਸ਼ੁਰੂ_STATEMENT_OR_RETURN(stmtp) \
    ਜੇ (!m_cfuncp) ਵਾਪਸ; \
    ਜੇ ((stmtp)->user1_ਸੈੱਟ_Once()) ਵਾਪਸ; \
    VL_RESTORER(m_ਸੌਂਪੋ_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_ਸੌਂਪੋ_Lhs = ਝੂਠ; \
    m_stmtp = (stmtp);

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਸ਼ੁਰੂ_STATEMENT_OR_RETURN(nodep);

        ਜੇ (Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed* ਅਚਲ packedp = VN_CAST(nodep->lhsp(), Cvt_Array_ਮੰਜ਼ਿਲ_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਵਿਅੰਜਕ_Lhsp = packedp->fromp()->unlink_Fr_ਪਿੱਠ();
            packedp->replaceWith(ਵਿਅੰਜਕ_Lhsp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(packedp), packedp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            nodep->rhsp(ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{rhsp->fileline(), rhsp, ਵਿਅੰਜਕ_Lhsp->dtypep()});
            nodep->dtypeFrom(ਵਿਅੰਜਕ_Lhsp);
        }
        // Direct assignment to a simple variable
        ਜੇ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ਗੰਢ* ਅਚਲ rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ਜੇ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ਵਾਪਸ;
            ਜੇ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (rhsp->ਚੌੜਾਈ_ਸ਼ਬਦ() * VL_EDATASIZE < STATIC_CONST_MIN_ਚੌੜਾਈ) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ਵਾਪਸ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ਅਚਲ ਤਾਰਕਿਕ isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ਜੇ (!isAtomic && rhsReadsLhs(nodep)) {
            create_ਆਰਜ਼ੀ(nodep->rhsp());
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        }

        m_ਸੌਂਪੋ_Lhs = ਸੱਚ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ਜੇ (AstVarRef* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ਜੇ (AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ਜੇ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        ਸ਼ੁਰੂ_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ਜੇ (v3Global.opt.autoflush()) {
            ਅਚਲ Ast_ਗੰਢ* searchp = nodep->nextp();
            ਜਦੋਂ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ਜੇ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_ਰੁੱਖ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ਨਹੀਂਤਾਂ {
                UINFO(4, "Autoflush " << nodep);
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(
                    ਨਵਾਂ AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ});
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        ਸ਼ੁਰੂ_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            create_ਆਰਜ਼ੀ(nodep->memp())->noSubst(ਸੱਚ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        ਸ਼ੁਰੂ_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ ਸ਼ੁਰੂ_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_L* nodep) override { ਦੇਖੋ_ਖਿਸਕਾਓ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_R* nodep) override { ਦੇਖੋ_ਖਿਸਕਾਓ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_RS* nodep) override { ਦੇਖੋ_ਖਿਸਕਾਓ(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override { check_ਗੰਢ(nodep); }
    // Operators
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Termop* nodep) override { check_ਗੰਢ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
        ਜੇ (!VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)) create_ਆਰਜ਼ੀ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ* nodep) override {
        iterateChildren(nodep);
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ਸੌਂਪੋ_Lhs);
            m_ਸੌਂਪੋ_Lhs = ਝੂਠ;
            iterate_And_ਅਗਲਾ_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ਸੌਂਪੋ_Lhs);
            m_ਸੌਂਪੋ_Lhs = ਝੂਠ;
            iterate_And_ਅਗਲਾ_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocSel* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ਸੌਂਪੋ_Lhs);
            m_ਸੌਂਪੋ_Lhs = ਝੂਠ;
            iterate_And_ਅਗਲਾ_Null(nodep->bitp());
        }
        check_ਗੰਢ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ਜੇ (!m_ਵਿੱਚ_Cons && nodep->is_Constant_ਰੁੱਖ()) {
            ਜੇ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::ਲੱਭੋ(nodep));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ++m_extracted_ਮੰਜ਼ਿਲ_Const_Pool;
            }
            ਵਾਪਸ;
        }
        VL_RESTORER(m_ਵਿੱਚ_Cons);
        m_ਵਿੱਚ_Cons = ਸੱਚ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    ਖਾਲੀ ਦੇਖੋ(Ast_Cons_Assoc_ਵਸਤੂ* nodep) override {
        ਜੇ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੇਲ_Masked* nodep) override {
        iterateChildren(nodep);
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* ਅਚਲ varp = create_ਆਰਜ਼ੀ(nodep);
            varp->noSubst(ਸੱਚ);  // Do not re-inline in V3Subst
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ਜੇ (needs_ਆਰਜ਼ੀ(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ਅਚਲ lRefp = is_Rhs_ਦਾ_ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(ਸੱਚ);

            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ਅਚਲ thenRefp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE};
            ਜੇ (lRefp) thenRefp->self_ਸੂਚਕ(lRefp->self_ਸੂਚਕ());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenExprp = nodep->thenp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਸੌਂਪੋ* ਅਚਲ thenAsspp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ਅਚਲ elseRefp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE};
            ਜੇ (lRefp) elseRefp->self_ਸੂਚਕ(lRefp->self_ਸੂਚਕ());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elseExprp = nodep->elsep()->unlink_Fr_ਪਿੱਠ();
            Ast_ਸੌਂਪੋ* ਅਚਲ elseAsspp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = nodep->condp()->unlink_Fr_ਪਿੱਠ();
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_ਰੁੱਖ();
            // Iterate the resulting assignments
            iterate(ifp);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp = nodep->exprsp();
        ਜੇ (nodep->ਵਿਅੰਜਕ_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
            ਲਈ (Ast_ਗੰਢ* argp = exprsp; argp; argp = argp->nextp()) {
                ਅਚਲ AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
                ਜੇ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ਅਚਲ varp = create_ਆਰਜ਼ੀ(nodep->exprsp());
                    varp->noSubst(ਸੱਚ);
                    ਰੋਕੋ;
                }
            }
        }
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);

            AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ subargp = fargp ? fargp->exprp() : argp;
            ਜੇ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ਅਚਲ ਮੁੱਲ_Varp = create_ਆਰਜ਼ੀ(subargp);
                ਮੁੱਲ_Varp->noSubst(ਸੱਚ);
                ਜੇ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ਅਚਲ nameVarp = create_ਆਰਜ਼ੀ(fargp->namep());
                    nameVarp->noSubst(ਸੱਚ);
                }
                ਜਾਰੀ;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ਜੇ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ਅਚਲ varp = create_ਆਰਜ਼ੀ(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(ਸੱਚ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Don't hit varrefs under vars
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_ਮੰਜ਼ਿਲ_Const_Pool);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

ਖਾਲੀ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("premit", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
