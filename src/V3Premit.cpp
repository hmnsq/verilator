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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Premit.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr పూర్ణాంకం STATIC_CONST_MIN_వెడల్పు = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

తరగతి PremitVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extracted_గమ్యం_Const_Pool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current block
    కొలత_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    Ast_కణుపు* m_stmtp = శూన్యసూచిక;  // Current statement
    తార్కిక m_కేటాయించు_Lhs = అబద్ధం;  // Inside assignment lhs, don't breakup extracts
    తార్కిక m_లోపల_Cons = అబద్ధం;  // Inside a NodeCons, so not the root of a value

    // METHODS
    తార్కిక needs_తాత్కాలిక(Ast_కణుపు_సమాసం* nodep) {
        // Consider adding a temp for this expression.
        ఒకవేళ (!m_stmtp) తిరిగి అబద్ధం;  // Not under a statement
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి అబద్ధం;  // Already processed
        ఒకవేళ (!nodep->isWide()) తిరిగి అబద్ధం;  // Not wide
        ఒకవేళ (m_కేటాయించు_Lhs) తిరిగి అబద్ధం;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->మొదటి_Abovep(), ArraySel), nodep, "Should have been ignored");
        తిరిగి నిజం;
    }

    శూన్య check_కణుపు(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (needs_తాత్కాలిక(nodep)) create_తాత్కాలిక(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    స్థిర తార్కిక useConstPool(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        uint64_t వెడల్పు = 0;
        తార్కిక has_దత్తాంశం = అబద్ధం;
        తార్కిక hasDynamic = అబద్ధం;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        స్థిరరాశి తార్కిక emittable = nodep->forall([&](స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
            ఒకవేళ (VN_IS(exprp, కణుపు_Cons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                తిరిగి నిజం;
            }
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(exprp, Const);
            ఒకవేళ (!constp) తిరిగి అబద్ధం;
            వెడల్పు += static_cast<uint64_t>(constp->వెడల్పు_పదాలు()) * VL_EDATASIZE;
            has_దత్తాంశం = నిజం;
            తిరిగి !constp->అంకె().isFourState();
        });
        ఒకవేళ (!emittable) తిరిగి అబద్ధం;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ఒకవేళ (hasDynamic) తిరిగి has_దత్తాంశం;
        తిరిగి వెడల్పు >= static_cast<uint64_t>(STATIC_CONST_MIN_వెడల్పు);
    }

    AstVarRef* is_Rhs_యొక్క_కేటాయించు_గమ్యం_Var(Ast_కణుపు_సమాసం* nodep) {
        // If enclosing statement is an assign
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_CAST(m_stmtp, కణుపు_కేటాయించు);
        ఒకవేళ (!assignp) తిరిగి శూన్యసూచిక;
        // of which 'nodep' is the RHS of
        ఒకవేళ (assignp->rhsp() != nodep) తిరిగి శూన్యసూచిక;
        // and the LHS is a sipmple variable reference
        AstVarRef* స్థిరరాశి lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ఒకవేళ (!lRefp) తిరిగి శూన్యసూచిక;
        AstVar* స్థిరరాశి varp = lRefp->varp();
        // And the RHS does not use the same variable
        ఒకవేళ (nodep->exists([varp](స్థిరరాశి AstVarRef* refp) { తిరిగి refp->varp() == varp; })) {
            తిరిగి శూన్యసూచిక;
        }
        // Then return the LHS reference
        తిరిగి lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(Ast_కణుపు_సమాసం* nodep) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        స్థిరరాశి std::string name = "__Vtemp_" + std::గమ్యం_string(++m_tmpVarCnt);
        AstVar* స్థిరరాశి varp = కొత్త AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(నిజం);
        varp->noReset(నిజం);
        m_cfuncp->కూడు_Varsp(varp);
        ++m_temporaryVarsCreated;
        తిరిగి varp;
    }

    AstVar* create_తాత్కాలిక(Ast_కణుపు_సమాసం* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlink_Fr_వెనుక(&relinker);

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstVar* varp = శూన్యసూచిక;

        ఒకవేళ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* స్థిరరాశి refp = V3ConstPool::కనుగొను(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            ++m_extracted_గమ్యం_Const_Pool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } లేకపోతే {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, varp, VAccess::WRITE};
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->కూడు_ఇక్కడ_This_As_తదుపరి(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(కొత్త AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        తిరిగి varp;
    }

    శూన్య replace_With_Ovr_జరుపు(Ast_కణుపు_Biop* nodep) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
        ఒకవేళ (VN_IS(nodep, జరుపు_L)) {
            newp = కొత్త Ast_జరుపు_LOvr{flp, lhsp, rhsp};
        } లేకపోతే ఒకవేళ (VN_IS(nodep, జరుపు_R)) {
            newp = కొత్త Ast_జరుపు_ROvr{flp, lhsp, rhsp};
        } లేకపోతే {
            newp = కొత్త Ast_జరుపు_RSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు_జరుపు(Ast_కణుపు_Biop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, జరుపు_L) || VN_IS(nodep, జరుపు_R) || VN_IS(nodep, జరుపు_RS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            స్థిరరాశి తార్కిక isOversized
                = shiftp->అంకె().most_సమితి_బిట్టు_P1() > 32
                  || (shiftp->అంకె().గమ్యం_UInt() >= static_cast<uint32_t>(nodep->వెడల్పు()));
            ఒకవేళ (isOversized) {
                // If signed shift, replace with replicated MSB
                ఒకవేళ (VN_IS(nodep, జరుపు_RS)) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
                    Ast_కణుపు_సమాసం* స్థిరరాశి msbp = కొత్త AstSel{flp, lhsp, lhsp->వెడల్పు_Min() - 1, 1};
                    nodep->replaceWithKeepDType(కొత్త AstExtendS{flp, msbp, nodep->వెడల్పు()});
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
                // Unsigned. If pure, replace with zero
                ఒకవేళ (nodep->lhsp()->is_శుద్ధ()) {
                    nodep->replaceWithKeepDType(
                        కొత్త AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
                // Impure. Keep shift
                ఒకవేళ (!nodep->isWide()) {
                    replace_With_Ovr_జరుపు(nodep);
                    తిరిగి;
                }
            }
        } లేకపోతే {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ఒకవేళ (!nodep->isWide() && (nodep->వెడల్పు() < (1LL << nodep->rhsp()->వెడల్పు_Min()))) {
                replace_With_Ovr_జరుపు(nodep);
                తిరిగి;
            }
        }
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }

    స్థిర తార్కిక rhsReadsLhs(స్థిరరాశి Ast_కణుపు_కేటాయించు* nodep) {
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
        nodep->lhsp()->foreach([](స్థిరరాశి AstVarRef* refp) {
            ఒకవేళ (refp->access().isWriteOrRW()) refp->varp()->user3(నిజం);
        });
        తిరిగి nodep->rhsp()->exists([](స్థిరరాశి AstVarRef* refp) {
            తిరిగి refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#నిర్వచించు ప్రారంభం_STATEMENT_OR_RETURN(stmtp) \
    ఒకవేళ (!m_cfuncp) తిరిగి; \
    ఒకవేళ ((stmtp)->user1_సమితి_Once()) తిరిగి; \
    VL_RESTORER(m_కేటాయించు_Lhs); \
    VL_RESTORER(m_stmtp); \
    m_కేటాయించు_Lhs = అబద్ధం; \
    m_stmtp = (stmtp);

    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ప్రారంభం_STATEMENT_OR_RETURN(nodep);

        ఒకవేళ (Ast_Cvt_Array_గమ్యం_Packed* స్థిరరాశి packedp = VN_CAST(nodep->lhsp(), Cvt_Array_గమ్యం_Packed)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            Ast_కణుపు_సమాసం* స్థిరరాశి సమాసం_Lhsp = packedp->fromp()->unlink_Fr_వెనుక();
            packedp->replaceWith(సమాసం_Lhsp);
            VL_DO_DANGLING(నెట్టు_Deletep(packedp), packedp);
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            nodep->rhsp(కొత్త Ast_Cvt_Packed_గమ్యం_Array{rhsp->fileline(), rhsp, సమాసం_Lhsp->dtypep()});
            nodep->dtypeFrom(సమాసం_Lhsp);
        }
        // Direct assignment to a simple variable
        ఒకవేళ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            Ast_కణుపు* స్థిరరాశి rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ఒకవేళ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) తిరిగి;
            ఒకవేళ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } లేకపోతే ఒకవేళ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(నిజం);
            } లేకపోతే ఒకవేళ (rhsp->వెడల్పు_పదాలు() * VL_EDATASIZE < STATIC_CONST_MIN_వెడల్పు) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                తిరిగి;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        స్థిరరాశి తార్కిక isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ఒకవేళ (!isAtomic && rhsReadsLhs(nodep)) {
            create_తాత్కాలిక(nodep->rhsp());
        } లేకపోతే {
            iterate_And_తదుపరి_Null(nodep->rhsp());
        }

        m_కేటాయించు_Lhs = నిజం;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterate_And_తదుపరి_Null(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ఒకవేళ (AstVarRef* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ఒకవేళ (AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ఒకవేళ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                }
            }
        }
    }
    శూన్య సందర్శించు(AstDisplay* nodep) override {
        ప్రారంభం_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ఒకవేళ (v3Global.opt.autoflush()) {
            స్థిరరాశి Ast_కణుపు* searchp = nodep->nextp();
            అయితే (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ఒకవేళ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->same_Gate_వృక్షం(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } లేకపోతే {
                UINFO(4, "Autoflush " << nodep);
                nodep->కూడు_తదుపరి_ఇక్కడ(
                    కొత్త AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->clone_వృక్షం_శుద్ధ(నిజం) : శూన్యసూచిక});
            }
        }
    }
    శూన్య సందర్శించు(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        ప్రారంభం_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(నిజం);
        } లేకపోతే {
            create_తాత్కాలిక(nodep->memp())->noSubst(నిజం);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        ప్రారంభం_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#నిర్వచనం_రద్దు ప్రారంభం_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    శూన్య సందర్శించు(Ast_జరుపు_L* nodep) override { సందర్శించు_జరుపు(nodep); }
    శూన్య సందర్శించు(Ast_జరుపు_R* nodep) override { సందర్శించు_జరుపు(nodep); }
    శూన్య సందర్శించు(Ast_జరుపు_RS* nodep) override { సందర్శించు_జరుపు(nodep); }

    శూన్య సందర్శించు(AstConst* nodep) override { check_కణుపు(nodep); }
    // Operators
    శూన్య సందర్శించు(Ast_కణుపు_Termop* nodep) override { check_కణుపు(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(AstRand* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(Ast_Cvt_Array_గమ్యం_Packed* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(Ast_Cvt_Packed_గమ్యం_Array* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
        ఒకవేళ (!VN_IS(nodep->backp(), కణుపు_కేటాయించు)) create_తాత్కాలిక(nodep);
    }
    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_వరుస* nodep) override {
        iterateChildren(nodep);
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        iterate_And_తదుపరి_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_కేటాయించు_Lhs);
            m_కేటాయించు_Lhs = అబద్ధం;
            iterate_And_తదుపరి_Null(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        iterate_And_తదుపరి_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_కేటాయించు_Lhs);
            m_కేటాయించు_Lhs = అబద్ధం;
            iterate_And_తదుపరి_Null(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    శూన్య సందర్శించు(AstAssocSel* nodep) override {
        iterate_And_తదుపరి_Null(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_కేటాయించు_Lhs);
            m_కేటాయించు_Lhs = అబద్ధం;
            iterate_And_తదుపరి_Null(nodep->bitp());
        }
        check_కణుపు(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ఒకవేళ (!m_లోపల_Cons && nodep->is_Constant_వృక్షం()) {
            ఒకవేళ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::కనుగొను(nodep));
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                ++m_extracted_గమ్యం_Const_Pool;
            }
            తిరిగి;
        }
        VL_RESTORER(m_లోపల_Cons);
        m_లోపల_Cons = నిజం;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    శూన్య సందర్శించు(Ast_Cons_Assoc_అంశం* nodep) override {
        ఒకవేళ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    శూన్య సందర్శించు(Ast_సరిపోలిక_Masked* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (!nodep->user1_సమితి_Once()) {
            // Don't want this replicated by V3Expand
            AstVar* స్థిరరాశి varp = create_తాత్కాలిక(nodep);
            varp->noSubst(నిజం);  // Do not re-inline in V3Subst
        }
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ఒకవేళ (needs_తాత్కాలిక(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* స్థిరరాశి lRefp = is_Rhs_యొక్క_కేటాయించు_గమ్యం_Var(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(నిజం);

            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* స్థిరరాశి thenRefp = కొత్త AstVarRef{flp, varp, VAccess::WRITE};
            ఒకవేళ (lRefp) thenRefp->self_సూచిక(lRefp->self_సూచిక());
            Ast_కణుపు_సమాసం* స్థిరరాశి thenExprp = nodep->thenp()->unlink_Fr_వెనుక();
            Ast_కేటాయించు* స్థిరరాశి thenAsspp = కొత్త Ast_కేటాయించు{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* స్థిరరాశి elseRefp = కొత్త AstVarRef{flp, varp, VAccess::WRITE};
            ఒకవేళ (lRefp) elseRefp->self_సూచిక(lRefp->self_సూచిక());
            Ast_కణుపు_సమాసం* స్థిరరాశి elseExprp = nodep->elsep()->unlink_Fr_వెనుక();
            Ast_కేటాయించు* స్థిరరాశి elseAsspp = కొత్త Ast_కేటాయించు{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            Ast_కణుపు_సమాసం* స్థిరరాశి condp = nodep->condp()->unlink_Fr_వెనుక();
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->కూడు_ఇక్కడ_This_As_తదుపరి(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clear_Cache_వృక్షం();
            // Iterate the resulting assignments
            iterate(ifp);
            తిరిగి;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        Ast_కణుపు_సమాసం* exprsp = nodep->exprsp();
        ఒకవేళ (nodep->సమాసం_Format_2()) {
            exprsp = VN_AS(exprsp->nextp(), కణుపు_సమాసం);
            కొరకు (Ast_కణుపు* argp = exprsp; argp; argp = argp->nextp()) {
                స్థిరరాశి AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
                ఒకవేళ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* స్థిరరాశి varp = create_తాత్కాలిక(nodep->exprsp());
                    varp->noSubst(నిజం);
                    ఆపు;
                }
            }
        }
        కొరకు (Ast_కణుపు_సమాసం *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), కణుపు_సమాసం);

            AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
            Ast_కణుపు_సమాసం* స్థిరరాశి subargp = fargp ? fargp->exprp() : argp;
            ఒకవేళ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* స్థిరరాశి విలువ_Varp = create_తాత్కాలిక(subargp);
                విలువ_Varp->noSubst(నిజం);
                ఒకవేళ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* స్థిరరాశి nameVarp = create_తాత్కాలిక(fargp->namep());
                    nameVarp->noSubst(నిజం);
                }
                కొనసాగు;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ఒకవేళ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* స్థిరరాశి varp = create_తాత్కాలిక(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(నిజం);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    శూన్య సందర్శించు(AstVar*) override {}  // Don't hit varrefs under vars
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన PremitVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~PremitVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Prelim extracted value to ConstPool",
                         m_extracted_గమ్యం_Const_Pool);
        V3Stats::కూడు_Stat("Optimizations, Prelim temporary variables created",
                         m_temporaryVarsCreated);
    }
};

//######################################################################
// Premit class functions

శూన్య V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("premit", 0, dump_వృక్షం_Either_Level() >= 3);
}
