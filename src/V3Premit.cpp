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

constexpr पूर्णाङ्क STATIC_CONST_MIN_विस्तारः = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

वर्ग PremitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_गन्तव्यम्_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = शून्यसूचकः;  // Current block
    आकार_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_ग्रन्थिः* m_stmtp = शून्यसूचकः;  // Current statement
    तार्किक m_नियोजय_Lhs = असत्य;  // Inside assignment lhs, don't breakup extracts
    तार्किक m_अन्तर्_Cons = असत्य;  // Inside a NodeCons, so not the root of a value

    // METHODS
    तार्किक needs_अस्थायी(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Consider adding a temp for this expression.
        यदि (!m_stmtp) प्रत्यागम असत्य;  // Not under a statement
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम असत्य;  // Already processed
        यदि (!nodep->isWide()) प्रत्यागम असत्य;  // Not wide
        यदि (m_नियोजय_Lhs) प्रत्यागम असत्य;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        प्रत्यागम सत्य;
    }

    शून्य check_ग्रन्थिः(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (needs_अस्थायी(nodep)) create_अस्थायी(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    स्थिर तार्किक useConstPool(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        uint64_t विस्तारः = 0;
        तार्किक has_दत्त = असत्य;
        तार्किक hasDynamic = असत्य;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        ध्रुव तार्किक emittable = nodep->forall([&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
            यदि (VN_IS(exprp, ग्रन्थिः_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                प्रत्यागम सत्य;
            }
            ध्रुव AstConst* ध्रुव constp = VN_CAST(exprp, Const);
            यदि (!constp) प्रत्यागम असत्य;
            विस्तारः += static_cast<uint64_t>(constp->विस्तारः_शब्दाः()) * VL_EDATASIZE;
            has_दत्त = सत्य;
            प्रत्यागम !constp->अङ्कः().isFourState();
        });
        यदि (!emittable) प्रत्यागम असत्य;
        // Dynamically sized values allocate when built with data, so use the pool for them
        यदि (hasDynamic) प्रत्यागम has_दत्त;
        प्रत्यागम विस्तारः >= static_cast<uint64_t>(STATIC_CONST_MIN_विस्तारः);
    }

    AstVarRef* is_Rhs_सम्बन्धि_नियोजय_गन्तव्यम्_Var(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // If enclosing statement is an assign
        Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = VN_CAST(m_stmtp, ग्रन्थिः_नियोजय);
        यदि (!assignp) प्रत्यागम शून्यसूचकः;
        // of which 'nodep' is the RHS of
        यदि (assignp->rhsp() != nodep) प्रत्यागम शून्यसूचकः;
        // and the LHS is a sipmple variable reference
        AstVarRef* ध्रुव lRefp = VN_CAST(assignp->lhsp(), VarRef);
        यदि (!lRefp) प्रत्यागम शून्यसूचकः;
        AstVar* ध्रुव varp = lRefp->varp();
        // And the RHS does not use the same variable
        यदि (nodep->exists([varp](ध्रुव AstVarRef* refp) { प्रत्यागम refp->varp() == varp; })) {
            प्रत्यागम शून्यसूचकः;
        }
        // Then return the LHS reference
        प्रत्यागम lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ध्रुव std::string name = "__Vtemp_" + std::गन्तव्यम्_string(++m_tmpVarCnt);
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(सत्य);
        varp->noReset(सत्य);
        m_cfuncp->सङ्कलय_Varsp(varp);
        ++m_temporaryVarsCreated;
        प्रत्यागम varp;
    }

    AstVar* create_अस्थायी(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_पृष्ठभागः(&relinker);

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstVar* varp = शून्यसूचकः;

        यदि (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* ध्रुव refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            ++m_extracted_गन्तव्यम्_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } अन्यथा {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->सङ्कलय_अत्र_This_As_अग्र(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(नूतन AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        प्रत्यागम varp;
    }

    शून्य replace_With_Ovr_सरणम्(Ast_ग्रन्थिः_Biop* nodep) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
        यदि (VN_IS(nodep, सरणम्_L)) {
            newp = नूतन Ast_सरणम्_LOvr{flp, lhsp, rhsp};
        } अन्यथा यदि (VN_IS(nodep, सरणम्_R)) {
            newp = नूतन Ast_सरणम्_ROvr{flp, lhsp, rhsp};
        } अन्यथा {
            newp = नूतन Ast_सरणम्_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय_सरणम्(Ast_ग्रन्थिः_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, सरणम्_L) || VN_IS(nodep, सरणम्_R) || VN_IS(nodep, सरणम्_RS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (ध्रुव AstConst* ध्रुव shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            ध्रुव तार्किक isOversized
                = shiftp->अङ्कः().most_समुच्चयः_Bit_P1() > 32
                  || (shiftp->अङ्कः().गन्तव्यम्_UInt() >= static_cast<uint32_t>(nodep->विस्तारः()));
            यदि (isOversized) {
                // If signed shift, replace with replicated MSB
                यदि (VN_IS(nodep, सरणम्_RS)) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव msbp = नूतन AstSel{flp, lhsp, lhsp->विस्तारः_Min() - 1, 1};
                    nodep->replaceWithKeepDType(नूतन AstExtendS{flp, msbp, nodep->विस्तारः()});
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                // Unsigned. If pure, replace with zero
                यदि (nodep->lhsp()->is_विशुद्धम्()) {
                    nodep->replaceWithKeepDType(
                        नूतन AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                // Impure. Keep shift
                यदि (!nodep->isWide()) {
                    replace_With_Ovr_सरणम्(nodep);
                    प्रत्यागम;
                }
            }
        } अन्यथा {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            यदि (!nodep->isWide() && (nodep->विस्तारः() < (1LL << nodep->rhsp()->विस्तारः_Min()))) {
                replace_With_Ovr_सरणम्(nodep);
                प्रत्यागम;
            }
        }
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }

    स्थिर तार्किक rhsReadsLhs(ध्रुव Ast_ग्रन्थिः_नियोजय* nodep) {
        ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
        nodep->lhsp()->foreach([](ध्रुव AstVarRef* refp) {
            यदि (refp->access().isWriteOrRW()) refp->varp()->user3(सत्य);
        });
        प्रत्यागम nodep->rhsp()->exists([](ध्रुव AstVarRef* refp) {
            प्रत्यागम refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    शून्य दर्शय(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#परिभाषय START_STATEMENT_OR_RETURN(stmtp) \
    यदि (!m_cfuncp) प्रत्यागम; \
    यदि ((stmtp)->user1_समुच्चयः_Once()) प्रत्यागम; \
    VL_RESTORER(m_नियोजय_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_नियोजय_Lhs = असत्य; \
    m_stmtp = (stmtp);

    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        यदि (Ast_Cvt_Array_गन्तव्यम्_Packed* ध्रुव packedp = VN_CAST(nodep->lhsp(), Cvt_Array_गन्तव्यम्_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम्_Lhsp = packedp->fromp()->unlink_Fr_पृष्ठभागः();
            packedp->replaceWith(व्यञ्जकम्_Lhsp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(packedp), packedp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            nodep->rhsp(नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{rhsp->fileline(), rhsp, व्यञ्जकम्_Lhsp->dtypep()});
            nodep->dtypeFrom(व्यञ्जकम्_Lhsp);
        }
        // Direct assignment to a simple variable
        यदि (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_ग्रन्थिः* ध्रुव rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            यदि (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) प्रत्यागम;
            यदि (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } अन्यथा यदि (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(सत्य);
            } अन्यथा यदि (rhsp->विस्तारः_शब्दाः() * VL_EDATASIZE < STATIC_CONST_MIN_विस्तारः) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                प्रत्यागम;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        ध्रुव तार्किक isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        यदि (!isAtomic && rhsReadsLhs(nodep)) {
            create_अस्थायी(nodep->rhsp());
        } अन्यथा {
            iterate_And_अग्र_Null(nodep->rhsp());
        }

        m_नियोजय_Lhs = सत्य;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_अग्र_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        यदि (AstVarRef* ध्रुव rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            यदि (AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                यदि (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                }
            }
        }
    }
    शून्य दर्शय(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        यदि (v3Global.opt.autoflush()) {
            ध्रुव Ast_ग्रन्थिः* searchp = nodep->nextp();
            यावत् (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            यदि (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_वृक्षः(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } अन्यथा {
                UINFO(4, "Autoflush " << nodep);
                nodep->सङ्कलय_अग्र_अत्र(
                    नूतन AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_वृक्षः_विशुद्धम्(सत्य) : शून्यसूचकः});
            }
        }
    }
    शून्य दर्शय(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(सत्य);
        } अन्यथा {
            create_अस्थायी(nodep->memp())->noSubst(सत्य);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#अपरिभाषय START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    शून्य दर्शय(Ast_सरणम्_L* nodep) override { दर्शय_सरणम्(nodep); }
    शून्य दर्शय(Ast_सरणम्_R* nodep) override { दर्शय_सरणम्(nodep); }
    शून्य दर्शय(Ast_सरणम्_RS* nodep) override { दर्शय_सरणम्(nodep); }

    शून्य दर्शय(AstConst* nodep) override { check_ग्रन्थिः(nodep); }
    // Operators
    शून्य दर्शय(Ast_ग्रन्थिः_Termop* nodep) override { check_ग्रन्थिः(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_Uniop* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(AstRand* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(Ast_Cvt_Array_गन्तव्यम्_Packed* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(Ast_Cvt_Packed_गन्तव्यम्_Array* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
        यदि (!VN_IS(nodep->backp(), ग्रन्थिः_नियोजय)) create_अस्थायी(nodep);
    }
    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Queue* nodep) override {
        iterateChildren(nodep);
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(AstSel* nodep) override {
        iterate_And_अग्र_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_नियोजय_Lhs);
            m_नियोजय_Lhs = असत्य;
            iterate_And_अग्र_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(AstArraySel* nodep) override {
        iterate_And_अग्र_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_नियोजय_Lhs);
            m_नियोजय_Lhs = असत्य;
            iterate_And_अग्र_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    शून्य दर्शय(AstAssocSel* nodep) override {
        iterate_And_अग्र_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_नियोजय_Lhs);
            m_नियोजय_Lhs = असत्य;
            iterate_And_अग्र_Null(nodep->bitp());
        }
        check_ग्रन्थिः(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        यदि (!m_अन्तर्_Cons && nodep->is_Constant_वृक्षः()) {
            यदि (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                ++m_extracted_गन्तव्यम्_Const_Pool;
            }
            प्रत्यागम;
        }
        VL_RESTORER(m_अन्तर्_Cons);
        m_अन्तर्_Cons = सत्य;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    शून्य दर्शय(AstConsAssocItem* nodep) override {
        यदि (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    शून्य दर्शय(Ast_मेलनम्_Masked* nodep) override {
        iterateChildren(nodep);
        यदि (!nodep->user1_समुच्चयः_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* ध्रुव varp = create_अस्थायी(nodep);
            varp->noSubst(सत्य);  // Do not re-inline in V3Subst
        }
    }
    शून्य दर्शय(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        यदि (needs_अस्थायी(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* ध्रुव lRefp = is_Rhs_सम्बन्धि_नियोजय_गन्तव्यम्_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(सत्य);

            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* ध्रुव thenRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            यदि (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenExprp = nodep->thenp()->unlink_Fr_पृष्ठभागः();
            Ast_नियोजय* ध्रुव thenAsspp = नूतन Ast_नियोजय{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* ध्रुव elseRefp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
            यदि (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elseExprp = nodep->elsep()->unlink_Fr_पृष्ठभागः();
            Ast_नियोजय* ध्रुव elseAsspp = नूतन Ast_नियोजय{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = nodep->condp()->unlink_Fr_पृष्ठभागः();
            AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->सङ्कलय_अत्र_This_As_अग्र(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_वृक्षः();
            // Iterate the resulting assignments
            iterate(ifp);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_ग्रन्थिः_व्यञ्जकम्* exprsp = nodep->exprsp();
        यदि (nodep->व्यञ्जकम्_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), ग्रन्थिः_व्यञ्जकम्);
            प्रति (Ast_ग्रन्थिः* argp = exprsp; argp; argp = argp->nextp()) {
                ध्रुव AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
                यदि (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* ध्रुव varp = create_अस्थायी(nodep->exprsp());
                    varp->noSubst(सत्य);
                    भङ्ग;
                }
            }
        }
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम् *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), ग्रन्थिः_व्यञ्जकम्);

            AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव subargp = fargp ? fargp->exprp() : argp;
            यदि (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* ध्रुव मानम्_Varp = create_अस्थायी(subargp);
                मानम्_Varp->noSubst(सत्य);
                यदि (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* ध्रुव nameVarp = create_अस्थायी(fargp->namep());
                    nameVarp->noSubst(सत्य);
                }
                अनुवर्त;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            यदि (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* ध्रुव varp = create_अस्थायी(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(सत्य);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    शून्य दर्शय(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::सङ्कलय_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_गन्तव्यम्_Const_Pool);
        V3Stats::सङ्कलय_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

शून्य V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("premit", 0, dump_वृक्षः_Either_Level() >= 3);
}
