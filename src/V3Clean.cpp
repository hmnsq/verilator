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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

വർഗ്ഗം CleanVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser3InUse m_inuser3;

    // TYPES
    എണ്ണൽ CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    സ്ഥിരം Ast_കണ്ണി_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    പൂർണ്ണസംഖ്യ cppWidth(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep->width() <= VL_IDATASIZE) {
            തിരികെ VL_IDATASIZE;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->width() <= VL_QUADSIZE) {
            തിരികെ VL_QUADSIZE;
        } അല്ലെങ്കിൽ {
            തിരികെ nodep->width_വാക്കുകൾ() * VL_EDATASIZE;
        }
    }
    ശൂന്യം setCppWidth(Ast_കണ്ണി* nodep) {
        nodep->user2(സത്യം);  // Don't resize it again
        Ast_കണ്ണി_DType* സ്ഥിരം old_dtypep = nodep->dtypep();
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = cppWidth(nodep);  // widthMin is unchanged
        എങ്കിൽ (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം new_dtypep = VN_CAST(old_dtypep->user3p(), കണ്ണി_DType)) {
                nodep->dtypep(new_dtypep);
            } അല്ലെങ്കിൽ {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_കണ്ണി_DType* സ്ഥിരം new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    ശൂന്യം computeCppWidth(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep->user2() && nodep->hasDType()) {
            എങ്കിൽ (VN_IS(nodep, Var)  //
                || VN_IS(nodep, കണ്ണി_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, കണ്ണി_Sel)  // Array selects should reflect variable widths
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
            } അല്ലെങ്കിൽ {
                സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), കണ്ണി_UOr_Struct_DType);
                എങ്കിൽ (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    ശൂന്യം setCleanState(Ast_കണ്ണി* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ static_cast<CleanState>(nodep->user1());
    }
    തർക്കം isClean(Ast_കണ്ണി* nodep) {
        സ്ഥിരം CleanState clstate = getCleanState(nodep);
        എങ്കിൽ (clstate == CS_CLEAN) തിരികെ സത്യം;
        എങ്കിൽ (clstate == CS_DIRTY) തിരികെ അസത്യം;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        തിരികെ അസത്യം;
    }
    ശൂന്യം setClean(Ast_കണ്ണി* nodep, തർക്കം isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        സ്ഥിരം തർക്കം wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    ശൂന്യം തിരുകുക_Clean(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_സംഖ്യ mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_കണ്ണി* സ്ഥിരം cleanp
            = പുതിയ AstAnd{nodep->fileline(), പുതിയ AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    ശൂന്യം ensureClean(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        computeCppWidth(nodep);
        എങ്കിൽ (!isClean(nodep)) തിരുകുക_Clean(nodep);
    }
    ശൂന്യം ensureCleanAndNext(Ast_കണ്ണി* nodep) {
        // Editing list, careful looping!
        വേണ്ടി (Ast_കണ്ണി* argp = nodep; argp;) {
            Ast_കണ്ണി* സ്ഥിരം nextp = argp->nextp();
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_CAST(argp, കണ്ണി_വ്യഞ്ജകം)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    ശൂന്യം operandBiop(Ast_കണ്ണി_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        എങ്കിൽ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        എങ്കിൽ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    ശൂന്യം operandTriop(Ast_കണ്ണി_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        എങ്കിൽ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        എങ്കിൽ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        എങ്കിൽ (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    ശൂന്യം operandQuadop(Ast_കണ്ണി_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        എങ്കിൽ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        എങ്കിൽ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        എങ്കിൽ (nodep->cleanThs()) ensureClean(nodep->thsp());
        എങ്കിൽ (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        എങ്കിൽ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ശൂന്യം visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    ശൂന്യം visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ശൂന്യം visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    ശൂന്യം visit(Ast_കണ്ണി_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        എങ്കിൽ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    ശൂന്യം visit(AstScopeName* nodep) override {  //
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, സത്യം);
    }
    // Constructor values are stored, so must be clean
    ശൂന്യം visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        എങ്കിൽ (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    ശൂന്യം visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, അസത്യം);
        // We always clean, as we don't trust those pesky users.
        എങ്കിൽ (!VN_IS(nodep->backp(), And)) തിരുകുക_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ശൂന്യം visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    ശൂന്യം visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    ശൂന്യം visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    ശൂന്യം visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    ശൂന്യം visit(AstLoop* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    ശൂന്യം visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, സത്യം);  // generates a string, so not relevant
    }
    ശൂന്യം visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ശൂന്യം visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    ശൂന്യം visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, സത്യം);
    }
    ശൂന്യം visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, സത്യം);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Clean class functions

ശൂന്യം V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("clean", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
