// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for clean nodes
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
// V3Clean's Transformations:
//
// Each module:
//      For each expression, if it requires a clean operand,
//      and the operand is dirty, insert a CLEAN node.
//      Resize operands (but not variables, variable selects, or formatting metadata)
//      to C++ 32/64/wide types.
//      Copy all width() values to widthMin() so RANGE, etc can still see orig widths
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

વર્ગ CleanVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser3InUse m_inuser3;

    // TYPES
    ગણના CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    અચલ Ast_ગાંઠ_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    પૂર્ણાંક cppWidth(અચલ Ast_ગાંઠ* nodep) {
        જો (nodep->width() <= VL_IDATASIZE) {
            પાછા VL_IDATASIZE;
        } નહીંતર જો (nodep->width() <= VL_QUADSIZE) {
            પાછા VL_QUADSIZE;
        } નહીંતર {
            પાછા nodep->width_શબ્દો() * VL_EDATASIZE;
        }
    }
    શૂન્ય setCppWidth(Ast_ગાંઠ* nodep) {
        nodep->user2(સાચું);  // Don't resize it again
        Ast_ગાંઠ_DType* અચલ old_dtypep = nodep->dtypep();
        અચલ પૂર્ણાંક width = cppWidth(nodep);  // widthMin is unchanged
        જો (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            જો (Ast_ગાંઠ_DType* અચલ new_dtypep = VN_CAST(old_dtypep->user3p(), ગાંઠ_DType)) {
                nodep->dtypep(new_dtypep);
            } નહીંતર {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_ગાંઠ_DType* અચલ new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    શૂન્ય computeCppWidth(Ast_ગાંઠ* nodep) {
        જો (!nodep->user2() && nodep->hasDType()) {
            જો (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ગાંઠ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ગાંઠ_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), QueueDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StreamDType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } નહીંતર {
                અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ગાંઠ_UOr_Struct_DType);
                જો (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    શૂન્ય setCleanState(Ast_ગાંઠ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(અચલ Ast_ગાંઠ* nodep) {
        પાછા static_cast<CleanState>(nodep->user1());
    }
    તાર્કિક isClean(Ast_ગાંઠ* nodep) {
        અચલ CleanState clstate = getCleanState(nodep);
        જો (clstate == CS_CLEAN) પાછા સાચું;
        જો (clstate == CS_DIRTY) પાછા ખોટું;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        પાછા ખોટું;
    }
    શૂન્ય setClean(Ast_ગાંઠ* nodep, તાર્કિક isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        અચલ તાર્કિક wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    શૂન્ય ઉમેરો_Clean(Ast_ગાંઠ_પદાવલિ* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_સંખ્યા mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_ગાંઠ* અચલ cleanp
            = નવું AstAnd{nodep->fileline(), નવું AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    શૂન્ય ensureClean(Ast_ગાંઠ_પદાવલિ* nodep) {
        computeCppWidth(nodep);
        જો (!isClean(nodep)) ઉમેરો_Clean(nodep);
    }
    શૂન્ય ensureCleanAndNext(Ast_ગાંઠ* nodep) {
        // Editing list, careful looping!
        માટે (Ast_ગાંઠ* argp = nodep; argp;) {
            Ast_ગાંઠ* અચલ nextp = argp->nextp();
            જો (Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_CAST(argp, ગાંઠ_પદાવલિ)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    શૂન્ય operandBiop(Ast_ગાંઠ_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        જો (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        જો (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    શૂન્ય operandTriop(Ast_ગાંઠ_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        જો (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        જો (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        જો (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    શૂન્ય operandQuadop(Ast_ગાંઠ_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        જો (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        જો (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        જો (nodep->cleanThs()) ensureClean(nodep->thsp());
        જો (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        જો (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    શૂન્ય visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    શૂન્ય visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    શૂન્ય visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    શૂન્ય visit(Ast_ગાંઠ_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        જો (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    શૂન્ય visit(AstScopeName* nodep) override {  //
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, સાચું);
    }
    // Constructor values are stored, so must be clean
    શૂન્ય visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        જો (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        જો (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        જો (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(Ast_ગાંઠ_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    શૂન્ય visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, ખોટું);
        // We always clean, as we don't trust those pesky users.
        જો (!VN_IS(nodep->backp(), And)) ઉમેરો_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    શૂન્ય visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    શૂન્ય visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    શૂન્ય visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    શૂન્ય visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    શૂન્ય visit(AstLoop* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    શૂન્ય visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, સાચું);  // generates a string, so not relevant
    }
    શૂન્ય visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    શૂન્ય visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    શૂન્ય visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, સાચું);
    }
    શૂન્ય visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, સાચું);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = મૂળભૂત;
};

//######################################################################
// Clean class functions

શૂન્ય V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("clean", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
