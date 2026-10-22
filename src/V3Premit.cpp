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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Premit.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ಪೂರ್ಣಾಂಕ STATIC_CONST_MIN_ಅಗಲ = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

ವರ್ಗ PremitVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_ಗಮ್ಯ_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current block
    ಗಾತ್ರ_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ಗಂಟು* m_stmtp = ಶೂನ್ಯಸೂಚಕ;  // Current statement
    ತಾರ್ಕಿಕ m_ನಿಯೋಜಿಸು_Lhs = ಸುಳ್ಳು;  // Inside assignment lhs, don't breakup extracts
    ತಾರ್ಕಿಕ m_ಒಳ_Cons = ಸುಳ್ಳು;  // Inside a NodeCons, so not the root of a value

    // METHODS
    ತಾರ್ಕಿಕ needs_ತಾತ್ಕಾಲಿಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Consider adding a temp for this expression.
        ಆದರೆ (!m_stmtp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Not under a statement
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Already processed
        ಆದರೆ (!nodep->isWide()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Not wide
        ಆದರೆ (m_ನಿಯೋಜಿಸು_Lhs) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->ಮೊದಲ_Abovep(), ArraySel), nodep, "Should have been ignored");
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ check_ಗಂಟು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (needs_ತಾತ್ಕಾಲಿಕ(nodep)) create_ತಾತ್ಕಾಲಿಕ(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    ಸ್ಥಿರ ತಾರ್ಕಿಕ useConstPool(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        uint64_t ಅಗಲ = 0;
        ತಾರ್ಕಿಕ has_ದತ್ತಾಂಶ = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ hasDynamic = ಸುಳ್ಳು;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ emittable = nodep->forall([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
            ಆದರೆ (VN_IS(exprp, ಗಂಟು_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(exprp, Const);
            ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಅಗಲ += static_cast<uint64_t>(constp->ಅಗಲ_ಪದಗಳು()) * VL_EDATASIZE;
            has_ದತ್ತಾಂಶ = ನಿಜ;
            ಹಿಂತಿರುಗಿ !constp->ಅಂಕೆ().isFourState();
        });
        ಆದರೆ (!emittable) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ಆದರೆ (hasDynamic) ಹಿಂತಿರುಗಿ has_ದತ್ತಾಂಶ;
        ಹಿಂತಿರುಗಿ ಅಗಲ >= static_cast<uint64_t>(STATIC_CONST_MIN_ಅಗಲ);
    }

    AstVarRef* is_Rhs_ದ_ನಿಯೋಜಿಸು_ಗಮ್ಯ_Var(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // If enclosing statement is an assign
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(m_stmtp, ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!assignp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // of which 'nodep' is the RHS of
        ಆದರೆ (assignp->rhsp() != nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // and the LHS is a sipmple variable reference
        AstVarRef* ಸ್ಥಿರಾಂಕ lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ಆದರೆ (!lRefp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVar* ಸ್ಥಿರಾಂಕ varp = lRefp->varp();
        // And the RHS does not use the same variable
        ಆದರೆ (nodep->exists([varp](ಸ್ಥಿರಾಂಕ AstVarRef* refp) { ಹಿಂತಿರುಗಿ refp->varp() == varp; })) {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // Then return the LHS reference
        ಹಿಂತಿರುಗಿ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಸ್ಥಿರಾಂಕ std::string name = "__Vtemp_" + std::ಗಮ್ಯ_string(++m_tmpVarCnt);
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(ನಿಜ);
        varp->noReset(ನಿಜ);
        m_cfuncp->ಕೂಡಿಸು_Varsp(varp);
        ++m_temporaryVarsCreated;
        ಹಿಂತಿರುಗಿ varp;
    }

    AstVar* create_ತಾತ್ಕಾಲಿಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstVar* varp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = V3ConstPool::ಕಂಡುಹಿಡಿ(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ++m_extracted_ಗಮ್ಯ_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE};
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ಹಿಂತಿರುಗಿ varp;
    }

    ಶೂನ್ಯ replace_With_Ovr_ಸರಿಸು(Ast_ಗಂಟು_Biop* nodep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_IS(nodep, ಸರಿಸು_L)) {
            newp = ಹೊಸ Ast_ಸರಿಸು_LOvr{flp, lhsp, rhsp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, ಸರಿಸು_R)) {
            newp = ಹೊಸ Ast_ಸರಿಸು_ROvr{flp, lhsp, rhsp};
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp = ಹೊಸ Ast_ಸರಿಸು_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ_ಸರಿಸು(Ast_ಗಂಟು_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ಸರಿಸು_L) || VN_IS(nodep, ಸರಿಸು_R) || VN_IS(nodep, ಸರಿಸು_RS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isOversized
                = shiftp->ಅಂಕೆ().most_ಗಣ_ಬಿಟ್_P1() > 32
                  || (shiftp->ಅಂಕೆ().ಗಮ್ಯ_UInt() >= static_cast<uint32_t>(nodep->ಅಗಲ()));
            ಆದರೆ (isOversized) {
                // If signed shift, replace with replicated MSB
                ಆದರೆ (VN_IS(nodep, ಸರಿಸು_RS)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ msbp = ಹೊಸ AstSel{flp, lhsp, lhsp->ಅಗಲ_Min() - 1, 1};
                    nodep->replaceWithKeepDType(ಹೊಸ AstExtendS{flp, msbp, nodep->ಅಗಲ()});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                // Unsigned. If pure, replace with zero
                ಆದರೆ (nodep->lhsp()->is_ಶುದ್ಧ()) {
                    nodep->replaceWithKeepDType(
                        ಹೊಸ AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
                // Impure. Keep shift
                ಆದರೆ (!nodep->isWide()) {
                    replace_With_Ovr_ಸರಿಸು(nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ಆದರೆ (!nodep->isWide() && (nodep->ಅಗಲ() < (1LL << nodep->rhsp()->ಅಗಲ_Min()))) {
                replace_With_Ovr_ಸರಿಸು(nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ rhsReadsLhs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) {
        ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
        nodep->lhsp()->foreach([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            ಆದರೆ (refp->access().isWriteOrRW()) refp->varp()->user3(ನಿಜ);
        });
        ಹಿಂತಿರುಗಿ nodep->rhsp()->exists([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            ಹಿಂತಿರುಗಿ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#ವ್ಯಾಖ್ಯಾನಿಸು ಆರಂಭ_STATEMENT_OR_RETURN(stmtp) \
    ಆದರೆ (!m_cfuncp) ಹಿಂತಿರುಗಿ; \
    ಆದರೆ ((stmtp)->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ; \
    VL_RESTORER(m_ನಿಯೋಜಿಸು_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_ನಿಯೋಜಿಸು_Lhs = ಸುಳ್ಳು; \
    m_stmtp = (stmtp);

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆರಂಭ_STATEMENT_OR_RETURN(nodep);

        ಆದರೆ (Ast_Cvt_Array_ಗಮ್ಯ_Packed* ಸ್ಥಿರಾಂಕ packedp = VN_CAST(nodep->lhsp(), Cvt_Array_ಗಮ್ಯ_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Lhsp = packedp->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
            packedp->replaceWith(ಅಭಿವ್ಯಕ್ತಿ_Lhsp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(packedp), packedp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->rhsp(ಹೊಸ Ast_Cvt_Packed_ಗಮ್ಯ_Array{rhsp->fileline(), rhsp, ಅಭಿವ್ಯಕ್ತಿ_Lhsp->dtypep()});
            nodep->dtypeFrom(ಅಭಿವ್ಯಕ್ತಿ_Lhsp);
        }
        // Direct assignment to a simple variable
        ಆದರೆ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ಆದರೆ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhsp->ಅಗಲ_ಪದಗಳು() * VL_EDATASIZE < STATIC_CONST_MIN_ಅಗಲ) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ಹಿಂತಿರುಗಿ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ಆದರೆ (!isAtomic && rhsReadsLhs(nodep)) {
            create_ತಾತ್ಕಾಲಿಕ(nodep->rhsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        }

        m_ನಿಯೋಜಿಸು_Lhs = ನಿಜ;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ಆದರೆ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        ಆರಂಭ_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ಆದರೆ (v3Global.opt.autoflush()) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* searchp = nodep->nextp();
            ಆಗ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ಆದರೆ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_ವೃಕ್ಷ(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(4, "Autoflush " << nodep);
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(
                    ಹೊಸ AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ});
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        ಆರಂಭ_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            create_ತಾತ್ಕಾಲಿಕ(nodep->memp())->noSubst(ನಿಜ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        ಆರಂಭ_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು ಆರಂಭ_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_L* nodep) override { ಭೇಟಿ_ಸರಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_R* nodep) override { ಭೇಟಿ_ಸರಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_RS* nodep) override { ಭೇಟಿ_ಸರಿಸು(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override { check_ಗಂಟು(nodep); }
    // Operators
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Termop* nodep) override { check_ಗಂಟು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Array_ಗಮ್ಯ_Packed* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Packed_ಗಮ್ಯ_Array* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
        ಆದರೆ (!VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)) create_ತಾತ್ಕಾಲಿಕ(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ* nodep) override {
        iterateChildren(nodep);
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ನಿಯೋಜಿಸು_Lhs);
            m_ನಿಯೋಜಿಸು_Lhs = ಸುಳ್ಳು;
            iterate_And_ಮುಂದಿನ_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ನಿಯೋಜಿಸು_Lhs);
            m_ನಿಯೋಜಿಸು_Lhs = ಸುಳ್ಳು;
            iterate_And_ಮುಂದಿನ_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocSel* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_ನಿಯೋಜಿಸು_Lhs);
            m_ನಿಯೋಜಿಸು_Lhs = ಸುಳ್ಳು;
            iterate_And_ಮುಂದಿನ_Null(nodep->bitp());
        }
        check_ಗಂಟು(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ಆದರೆ (!m_ಒಳ_Cons && nodep->is_Constant_ವೃಕ್ಷ()) {
            ಆದರೆ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::ಕಂಡುಹಿಡಿ(nodep));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ++m_extracted_ಗಮ್ಯ_Const_Pool;
            }
            ಹಿಂತಿರುಗಿ;
        }
        VL_RESTORER(m_ಒಳ_Cons);
        m_ಒಳ_Cons = ನಿಜ;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cons_Assoc_ಅಂಶ* nodep) override {
        ಆದರೆ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹೊಂದಿಕೆ_Masked* nodep) override {
        iterateChildren(nodep);
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* ಸ್ಥಿರಾಂಕ varp = create_ತಾತ್ಕಾಲಿಕ(nodep);
            varp->noSubst(ನಿಜ);  // Do not re-inline in V3Subst
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ಆದರೆ (needs_ತಾತ್ಕಾಲಿಕ(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ಸ್ಥಿರಾಂಕ lRefp = is_Rhs_ದ_ನಿಯೋಜಿಸು_ಗಮ್ಯ_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(ನಿಜ);

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ಸ್ಥಿರಾಂಕ thenRefp = ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE};
            ಆದರೆ (lRefp) thenRefp->self_ಸೂಚಕ(lRefp->self_ಸೂಚಕ());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenExprp = nodep->thenp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ thenAsspp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ಸ್ಥಿರಾಂಕ elseRefp = ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE};
            ಆದರೆ (lRefp) elseRefp->self_ಸೂಚಕ(lRefp->self_ಸೂಚಕ());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elseExprp = nodep->elsep()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ elseAsspp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_ವೃಕ್ಷ();
            // Iterate the resulting assignments
            iterate(ifp);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp = nodep->exprsp();
        ಆದರೆ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಗಾಗಿ (Ast_ಗಂಟು* argp = exprsp; argp; argp = argp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
                ಆದರೆ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ಸ್ಥಿರಾಂಕ varp = create_ತಾತ್ಕಾಲಿಕ(nodep->exprsp());
                    varp->noSubst(ನಿಜ);
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);

            AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ subargp = fargp ? fargp->exprp() : argp;
            ಆದರೆ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_Varp = create_ತಾತ್ಕಾಲಿಕ(subargp);
                ಮೌಲ್ಯ_Varp->noSubst(ನಿಜ);
                ಆದರೆ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ಸ್ಥಿರಾಂಕ nameVarp = create_ತಾತ್ಕಾಲಿಕ(fargp->namep());
                    nameVarp->noSubst(ನಿಜ);
                }
                ಮುಂದುವರಿ;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ಆದರೆ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = create_ತಾತ್ಕಾಲಿಕ(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(ನಿಜ);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Don't hit varrefs under vars
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_ಗಮ್ಯ_Const_Pool);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

ಶೂನ್ಯ V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("premit", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
