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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Premit.h"

#include "V3ConstPool.h"
#include "V3Stats.h"
#include "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ຈຳນວນເຕັມ STATIC_CONST_MIN_WIDTH = 256;  // Minimum size to extract to static constant

//######################################################################
// Premit state, as a visitor of each AstNode

class PremitVisitor final : public VNVisitor {
    // NODE STATE
    //  AstNodeExpr::user()     -> bool.  True if iterated already
    //  *::user3()              -> bool.  Used when visiting AstNodeAssign
    const VNUser1InUse m_inuser1;

    // STATE - across all visitors
    VDouble0 m_extractedToConstPool;  // Statistic tracking
    VDouble0 m_temporaryVarsCreated;  // Statistic tracking

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    size_t m_tmpVarCnt = 0;  // Number of temporary variables created inside a function
    AstNode* m_stmtp = nullptr;  // Current statement
    bool m_assignLhs = false;  // Inside assignment lhs, don't breakup extracts
    bool m_inCons = false;  // Inside a NodeCons, so not the root of a value

    // METHODS
    bool needsTemp(AstNodeExpr* nodep) {
        // Consider adding a temp for this expression.
        ຖ້າ (!m_stmtp) ສົ່ງຄືນ false;  // Not under a statement
        ຖ້າ (nodep->user1SetOnce()) ສົ່ງຄືນ false;  // Already processed
        ຖ້າ (!nodep->isWide()) ສົ່ງຄືນ false;  // Not wide
        ຖ້າ (m_assignLhs) ສົ່ງຄືນ false;  // This is an lvalue!
        UASSERT_OBJ(!VN_IS(nodep->firstAbovep(), ArraySel), nodep, "Should have been ignored");
        ສົ່ງຄືນ true;
    }

    void checkNode(AstNodeExpr* nodep) {
        ຖ້າ (needsTemp(nodep)) createTemp(nodep);
    }

    // True if the value should be in the constant pool, rather than built where it is used
    static bool useConstPool(const AstNodeExpr* nodep) {
        uint64_t width = 0;
        bool hasData = false;
        bool hasDynamic = false;
        // Must be a tree of constants (see AstNodeExpr::isConstantTree), which can be emitted
        const bool emittable = nodep->forall([&](const AstNodeExpr* exprp) {
            ຖ້າ (VN_IS(exprp, NodeCons)) {
                hasDynamic |= exprp->dtypep()->skipRefp()->isDynamicallySized();
                ສົ່ງຄືນ true;
            }
            const AstConst* const constp = VN_CAST(exprp, Const);
            ຖ້າ (!constp) ສົ່ງຄືນ false;
            width += static_cast<uint64_t>(constp->widthWords()) * VL_EDATASIZE;
            hasData = true;
            ສົ່ງຄືນ !constp->num().isFourState();
        });
        ຖ້າ (!emittable) ສົ່ງຄືນ false;
        // Dynamically sized values allocate when built with data, so use the pool for them
        ຖ້າ (hasDynamic) ສົ່ງຄືນ hasData;
        ສົ່ງຄືນ width >= static_cast<uint64_t>(STATIC_CONST_MIN_WIDTH);
    }

    AstVarRef* isRhsOfAssignToVar(AstNodeExpr* nodep) {
        // If enclosing statement is an assign
        AstNodeAssign* const assignp = VN_CAST(m_stmtp, NodeAssign);
        ຖ້າ (!assignp) ສົ່ງຄືນ nullptr;
        // of which 'nodep' is the RHS of
        ຖ້າ (assignp->rhsp() != nodep) ສົ່ງຄືນ nullptr;
        // and the LHS is a sipmple variable reference
        AstVarRef* const lRefp = VN_CAST(assignp->lhsp(), VarRef);
        ຖ້າ (!lRefp) ສົ່ງຄືນ nullptr;
        AstVar* const varp = lRefp->varp();
        // And the RHS does not use the same variable
        ຖ້າ (nodep->exists([varp](const AstVarRef* refp) { ສົ່ງຄືນ refp->varp() == varp; })) {
            ສົ່ງຄືນ nullptr;
        }
        // Then return the LHS reference
        ສົ່ງຄືນ lRefp;
    }

    // Create a new temporary that can hold the value of the given expression
    AstVar* newTmpFor(AstNodeExpr* nodep) {
        FileLine* const flp = nodep->fileline();
        const std::string name = "__Vtemp_" + std::to_string(++m_tmpVarCnt);
        AstVar* const varp = new AstVar{flp, VVarType::STMTTEMP, name, nodep->dtypep()};
        varp->funcLocal(true);
        varp->noReset(true);
        m_cfuncp->addVarsp(varp);
        ++m_temporaryVarsCreated;
        ສົ່ງຄືນ varp;
    }

    AstVar* createTemp(AstNodeExpr* nodep) {
        UASSERT_OBJ(m_stmtp, nodep, "Attempting to create temporary with no insertion point");
        UINFO(4, "createTemp: " << nodep);

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);

        FileLine* const flp = nodep->fileline();
        AstVar* varp = nullptr;

        ຖ້າ (useConstPool(nodep)) {
            // Extract into constant pool.
            AstVarRef* const refp = V3ConstPool::find(nodep);
            varp = refp->varp();
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            ++m_extractedToConstPool;
            // Replace node with reference to the constant pool entry
            relinker.relink(refp);
        } ບໍ່ດັ່ງນັ້ນ {
            // Keep as local temporary.
            varp = newTmpFor(nodep);
            // Assignment to put before the referencing statement
            AstVarRef* const refp = new AstVarRef{flp, varp, VAccess::WRITE};
            AstAssign* const assignp = new AstAssign{flp, refp, nodep};
            // Insert before the statement
            m_stmtp->addHereThisAsNext(assignp);
            // Replace node with VarRef to new Var
            relinker.relink(new AstVarRef{flp, varp, VAccess::READ});
        }

        // Return the temporary variable
        ສົ່ງຄືນ varp;
    }

    void replaceWithOvrShift(AstNodeBiop* nodep) {
        FileLine* const flp = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeExpr* newp = nullptr;
        ຖ້າ (VN_IS(nodep, ShiftL)) {
            newp = new AstShiftLOvr{flp, lhsp, rhsp};
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (VN_IS(nodep, ShiftR)) {
            newp = new AstShiftROvr{flp, lhsp, rhsp};
        } ບໍ່ດັ່ງນັ້ນ {
            newp = new AstShiftRSOvr{flp, lhsp, rhsp};
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visitShift(AstNodeBiop* nodep) {
        UINFO(4, "  ShiftFix  " << nodep);
        UASSERT_OBJ(VN_IS(nodep, ShiftL) || VN_IS(nodep, ShiftR) || VN_IS(nodep, ShiftRS), nodep,
                    "Bad case");
        // Shift larger than the width of the type (overshift) is undefined behavour in C++
        // (in practice will shift by the wrapped shift amount). These are requierd to go to
        // zero/msbs, so replacing them here.
        FileLine* const flp = nodep->fileline();
        ຖ້າ (const AstConst* const shiftp = VN_CAST(nodep->rhsp(), Const)) {
            // Shift amount known to be constant. If oversized shift, replace with zero/msbs.
            // Otherwise we can leave the original shifts which have better constant folding
            // than the *Ovr versions.
            const bool isOversized
                = shiftp->num().mostSetBitP1() > 32
                  || (shiftp->num().toUInt() >= static_cast<uint32_t>(nodep->width()));
            ຖ້າ (isOversized) {
                // If signed shift, replace with replicated MSB
                ຖ້າ (VN_IS(nodep, ShiftRS)) {
                    AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
                    AstNodeExpr* const msbp = new AstSel{flp, lhsp, lhsp->widthMin() - 1, 1};
                    nodep->replaceWithKeepDType(new AstExtendS{flp, msbp, nodep->width()});
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ສົ່ງຄືນ;
                }
                // Unsigned. If pure, replace with zero
                ຖ້າ (nodep->lhsp()->isPure()) {
                    nodep->replaceWithKeepDType(
                        new AstConst{flp, AstConst::DTyped{}, nodep->dtypep()});
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ສົ່ງຄືນ;
                }
                // Impure. Keep shift
                ຖ້າ (!nodep->isWide()) {
                    replaceWithOvrShift(nodep);
                    ສົ່ງຄືນ;
                }
            }
        } ບໍ່ດັ່ງນັ້ນ {
            // Shift amount not known at compile time. Convert to *Ovr version. Don't need to do
            // if it would use a wide operation which works correctly at runtime, of if the max
            // value of the shift amount is less than the with of the shifted value.
            ຖ້າ (!nodep->isWide() && (nodep->width() < (1LL << nodep->rhsp()->widthMin()))) {
                replaceWithOvrShift(nodep);
                ສົ່ງຄືນ;
            }
        }
        iterateChildren(nodep);
        checkNode(nodep);
    }

    static bool rhsReadsLhs(const AstNodeAssign* nodep) {
        const VNUser3InUse user3InUse;
        nodep->lhsp()->foreach([](const AstVarRef* refp) {
            ຖ້າ (refp->access().isWriteOrRW()) refp->varp()->user3(true);
        });
        ສົ່ງຄືນ nodep->rhsp()->exists([](const AstVarRef* refp) {
            ສົ່ງຄືນ refp->access().isReadOnly() && refp->varp()->user3();
        });
    }

    // VISITORS
    void visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_cfuncp, nodep, "Should not nest");
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_tmpVarCnt);
        m_cfuncp = nodep;
        m_tmpVarCnt = 0;
        iterateChildren(nodep);
    }

    // VISITORS - Statements
#define START_STATEMENT_OR_RETURN(stmtp) \
    ຖ້າ (!m_cfuncp) ສົ່ງຄືນ; \
    ຖ້າ ((stmtp)->user1SetOnce()) ສົ່ງຄືນ; \
    VL_RESTORER(m_assignLhs); \
    VL_RESTORER(m_stmtp); \
    m_assignLhs = false; \
    m_stmtp = (stmtp);

    void visit(AstNodeAssign* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);

        ຖ້າ (AstCvtArrayToPacked* const packedp = VN_CAST(nodep->lhsp(), CvtArrayToPacked)) {
            // AstCvtArrayToPacked is converted to VL_PACK, which returns rvalue,
            // so it shouldn't be on the LHS. It is now replaced with unpacking of RHS.
            AstNodeExpr* const exprLhsp = packedp->fromp()->unlinkFrBack();
            packedp->replaceWith(exprLhsp);
            VL_DO_DANGLING(pushDeletep(packedp), packedp);
            AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
            nodep->rhsp(new AstCvtPackedToArray{rhsp->fileline(), rhsp, exprLhsp->dtypep()});
            nodep->dtypeFrom(exprLhsp);
        }
        // Direct assignment to a simple variable
        ຖ້າ (VN_IS(nodep->lhsp(), VarRef) && !AstVar::scVarRecurse(nodep->lhsp())) {
            AstNode* const rhsp = nodep->rhsp();
            // Rhs is already a var ref, so nothing to do
            ຖ້າ (VN_IS(rhsp, VarRef) && !AstVar::scVarRecurse(rhsp)) ສົ່ງຄືນ;
            ຖ້າ (VN_IS(rhsp, Cond)) {
                // Do replace Cond on RHS, even if a simple assignment
            } ບໍ່ດັ່ງນັ້ນ ຖ້າ (!VN_IS(rhsp, Const)) {
                // Don't replace the rhs, it's already a simple assignment
                rhsp->user1(true);
            } ບໍ່ດັ່ງນັ້ນ ຖ້າ (rhsp->widthWords() * VL_EDATASIZE < STATIC_CONST_MIN_WIDTH) {
                // It's a small constant, so nothing to do, otherwise will be put to const pool
                ສົ່ງຄືນ;
            }
        }

        // If the RHS reads the LHS, we need a temporary unless the update is atomic
        const bool isAtomic = VN_IS(nodep->lhsp(), VarRef) && !nodep->lhsp()->isWide();
        ຖ້າ (!isAtomic && rhsReadsLhs(nodep)) {
            createTemp(nodep->rhsp());
        } ບໍ່ດັ່ງນັ້ນ {
            iterateAndNextNull(nodep->rhsp());
        }

        m_assignLhs = true;  // Restored by VL_RESTORER in START_STATEMENT_OR_RETURN
        iterateAndNextNull(nodep->lhsp());

        // It's possible we end up with an 'a' = 'a' after expanding an AstCond
        ຖ້າ (AstVarRef* const rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
            ຖ້າ (AstVarRef* const lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
                ຖ້າ (lhsp->varp() == rhsp->varp()) {
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                }
            }
        }
    }
    void visit(AstDisplay* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        ຖ້າ (v3Global.opt.autoflush()) {
            const AstNode* searchp = nodep->nextp();
            ໃນຂະນະທີ່ (searchp && VN_IS(searchp, Comment)) searchp = searchp->nextp();
            ຖ້າ (searchp && VN_IS(searchp, Display)
                && nodep->filep()->sameGateTree(VN_AS(searchp, Display)->filep())) {
                // There's another display next; we can just wait to flush
            } ບໍ່ດັ່ງນັ້ນ {
                UINFO(4, "Autoflush " << nodep);
                nodep->addNextHere(
                    new AstFFlush{nodep->fileline(),
                                  nodep->filep() ? nodep->filep()->cloneTreePure(true) : nullptr});
            }
        }
    }
    void visit(AstWriteMem* nodep) override {
        // Task lowering and scheduling put every memory dump inside a C++ function.
        UASSERT_OBJ(m_cfuncp, nodep, "Memory dump not under CFunc");
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
        // Memory dumps take an address, so force-aware reads need stable storage.
        // Keep temporaries created here or while visiting a wide memory expression.
        ຖ້າ (const AstVarRef* const refp = VN_CAST(nodep->memp(), VarRef)) {
            refp->varp()->noSubst(true);
        } ບໍ່ດັ່ງນັ້ນ {
            createTemp(nodep->memp())->noSubst(true);
        }
    }
    void visit(AstNodeStmt* nodep) override {
        START_STATEMENT_OR_RETURN(nodep);
        iterateChildren(nodep);
    }

#undef START_STATEMENT_OR_RETURN

    // VISITORS - Expressions
    void visit(AstShiftL* nodep) override { visitShift(nodep); }
    void visit(AstShiftR* nodep) override { visitShift(nodep); }
    void visit(AstShiftRS* nodep) override { visitShift(nodep); }

    void visit(AstConst* nodep) override { checkNode(nodep); }
    // Operators
    void visit(AstNodeTermop* nodep) override { checkNode(nodep); }
    void visit(AstNodeUniop* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstNodeBiop* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstRand* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstRandRNG* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstCvtArrayToPacked* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstCvtPackedToArray* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
        ຖ້າ (!VN_IS(nodep->backp(), NodeAssign)) createTemp(nodep);
    }
    void visit(AstCvtUnpackToQueue* nodep) override {
        iterateChildren(nodep);
        checkNode(nodep);
    }
    void visit(AstSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = false;
            iterateAndNextNull(nodep->lsbp());
            // AstSel::widthp() must remain a constant, so not iterating
        }
        checkNode(nodep);
    }
    void visit(AstArraySel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = false;
            iterateAndNextNull(nodep->bitp());
        }
        // ArraySel are just pointer arithmetic and should never be replaced
    }
    void visit(AstAssocSel* nodep) override {
        iterateAndNextNull(nodep->fromp());
        {  // Only the 'from' is part of the assignment LHS
            VL_RESTORER(m_assignLhs);
            m_assignLhs = false;
            iterateAndNextNull(nodep->bitp());
        }
        checkNode(nodep);
    }
    void visit(AstNodeCons* nodep) override {
        // Only lift whole values, as editing the parts of a constant would make it non-constant
        ຖ້າ (!m_inCons && nodep->isConstantTree()) {
            ຖ້າ (m_stmtp && useConstPool(nodep)) {
                nodep->replaceWith(V3ConstPool::find(nodep));
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ++m_extractedToConstPool;
            }
            ສົ່ງຄືນ;
        }
        VL_RESTORER(m_inCons);
        m_inCons = true;
        iterateChildren(nodep);
    }
    // Const associative array keys are emitted as initializers, so need no temporary
    void visit(AstConsAssocItem* nodep) override {
        ຖ້າ (!VN_IS(nodep->keyp(), Const)) iterate(nodep->keyp());
        iterate(nodep->valuep());
    }
    void visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ຖ້າ (!nodep->user1SetOnce()) {
            // Don't want this replicated by V3Expand
            AstVar* const varp = createTemp(nodep);
            varp->noSubst(true);  // Do not re-inline in V3Subst
        }
    }
    void visit(AstCond* nodep) override {
        // Convert AstCond to AstIf in order to avoid evaluating
        // sub-expressions in both branches unconditionally.
        ຖ້າ (needsTemp(nodep)) {
            // Check if LHS variable could be used directly
            AstVarRef* const lRefp = isRhsOfAssignToVar(nodep);
            // If not, create a new temporary variable
            AstVar* varp = lRefp ? lRefp->varp() : newTmpFor(nodep);
            // Can't substitute across basic blocks
            varp->noSubst(true);

            FileLine* const flp = nodep->fileline();
            // Create 'then' assignment
            AstVarRef* const thenRefp = new AstVarRef{flp, varp, VAccess::WRITE};
            ຖ້າ (lRefp) thenRefp->selfPointer(lRefp->selfPointer());
            AstNodeExpr* const thenExprp = nodep->thenp()->unlinkFrBack();
            AstAssign* const thenAsspp = new AstAssign{flp, thenRefp, thenExprp};
            // Create 'else' assignment
            AstVarRef* const elseRefp = new AstVarRef{flp, varp, VAccess::WRITE};
            ຖ້າ (lRefp) elseRefp->selfPointer(lRefp->selfPointer());
            AstNodeExpr* const elseExprp = nodep->elsep()->unlinkFrBack();
            AstAssign* const elseAsspp = new AstAssign{flp, elseRefp, elseExprp};
            // Creae 'if' and insert it before the statement
            AstNodeExpr* const condp = nodep->condp()->unlinkFrBack();
            AstIf* const ifp = new AstIf{flp, condp, thenAsspp, elseAsspp};
            m_stmtp->addHereThisAsNext(ifp);
            // Replace the AstCond with the result variable
            nodep->replaceWith(new AstVarRef{flp, varp, VAccess::READ});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            // Splitting to multiple statements can change purity
            VIsCached::clearCacheTree();
            // Iterate the resulting assignments
            iterate(ifp);
            ສົ່ງຄືນ;
        }
        iterateChildren(nodep);
    }
    void visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        // Any strings sent to a display must be var of string data type,
        // to avoid passing a pointer to a temporary.
        AstNodeExpr* exprsp = nodep->exprsp();
        ຖ້າ (nodep->exprFormat()) {
            exprsp = VN_AS(exprsp->nextp(), NodeExpr);
            ສຳລັບ (AstNode* argp = exprsp; argp; argp = argp->nextp()) {
                const AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
                ຖ້າ (fargp && fargp->formatAttr().isEnum()) {
                    // Evaluate the format before materializing enum arguments.
                    AstVar* const varp = createTemp(nodep->exprsp());
                    varp->noSubst(true);
                    bha_nga;
                }
            }
        }
        ສຳລັບ (AstNodeExpr *argp = exprsp, *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), NodeExpr);

            AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
            AstNodeExpr* const subargp = fargp ? fargp->exprp() : argp;
            ຖ້າ (fargp && fargp->formatAttr().isEnum()) {
                // The name lookup must see the same value as the numeric argument.
                AstVar* const valueVarp = createTemp(subargp);
                valueVarp->noSubst(true);
                ຖ້າ (!VN_IS(fargp->namep(), VarRef)) {
                    AstVar* const nameVarp = createTemp(fargp->namep());
                    nameVarp->noSubst(true);
                }
                continue;
            }
            // Must avoid taking address of rvalue, so even Const needs a temp
            ຖ້າ (subargp->isString() && !VN_IS(subargp, VarRef)) {
                AstVar* const varp = createTemp(subargp);
                // Do not remove VarRefs to this in V3Const
                varp->noSubst(true);
            }
        }
    }

    //--------------------
    // Default: Just iterate
    void visit(AstVar*) override {}  // Don't hit varrefs under vars
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
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

void V3Premit::premitAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { PremitVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("premit", 0, dumpTreeEitherLevel() >= 3);
}
