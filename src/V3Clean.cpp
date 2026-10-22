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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

वर्ग CleanVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिरांक VNUser3InUse m_inuser3;

    // TYPES
    गणना CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    स्थिरांक Ast_गाठ_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    पूर्णांक cppWidth(स्थिरांक Ast_गाठ* nodep) {
        जर (nodep->width() <= VL_IDATASIZE) {
            परत VL_IDATASIZE;
        } अन्यथा जर (nodep->width() <= VL_QUADSIZE) {
            परत VL_QUADSIZE;
        } अन्यथा {
            परत nodep->width_शब्द() * VL_EDATASIZE;
        }
    }
    शून्य setCppWidth(Ast_गाठ* nodep) {
        nodep->user2(सत्य);  // Don't resize it again
        Ast_गाठ_DType* स्थिरांक old_dtypep = nodep->dtypep();
        स्थिरांक पूर्णांक width = cppWidth(nodep);  // widthMin is unchanged
        जर (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            जर (Ast_गाठ_DType* स्थिरांक new_dtypep = VN_CAST(old_dtypep->user3p(), गाठ_DType)) {
                nodep->dtypep(new_dtypep);
            } अन्यथा {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_गाठ_DType* स्थिरांक new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    शून्य computeCppWidth(Ast_गाठ* nodep) {
        जर (!nodep->user2() && nodep->hasDType()) {
            जर (VN_IS(nodep, Var)  //
                || VN_IS(nodep, गाठ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, गाठ_Sel)  // Array selects should reflect variable widths
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
            } अन्यथा {
                स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), गाठ_UOr_Struct_DType);
                जर (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    शून्य setCleanState(Ast_गाठ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(स्थिरांक Ast_गाठ* nodep) {
        परत static_cast<CleanState>(nodep->user1());
    }
    तार्किक isClean(Ast_गाठ* nodep) {
        स्थिरांक CleanState clstate = getCleanState(nodep);
        जर (clstate == CS_CLEAN) परत सत्य;
        जर (clstate == CS_DIRTY) परत असत्य;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        परत असत्य;
    }
    शून्य setClean(Ast_गाठ* nodep, तार्किक isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        स्थिरांक तार्किक wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    शून्य घाला_Clean(Ast_गाठ_अभिव्यक्ती* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_संख्या mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_गाठ* स्थिरांक cleanp
            = नवीन AstAnd{nodep->fileline(), नवीन AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    शून्य ensureClean(Ast_गाठ_अभिव्यक्ती* nodep) {
        computeCppWidth(nodep);
        जर (!isClean(nodep)) घाला_Clean(nodep);
    }
    शून्य ensureCleanAndNext(Ast_गाठ* nodep) {
        // Editing list, careful looping!
        साठी (Ast_गाठ* argp = nodep; argp;) {
            Ast_गाठ* स्थिरांक nextp = argp->nextp();
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_CAST(argp, गाठ_अभिव्यक्ती)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    शून्य operandBiop(Ast_गाठ_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        जर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        जर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य operandTriop(Ast_गाठ_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        जर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        जर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        जर (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य operandQuadop(Ast_गाठ_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        जर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        जर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        जर (nodep->cleanThs()) ensureClean(nodep->thsp());
        जर (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        जर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_गाठ_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    शून्य visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    शून्य visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    शून्य visit(Ast_गाठ_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        जर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    शून्य visit(AstScopeName* nodep) override {  //
        setClean(nodep, सत्य);
    }
    शून्य visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सत्य);
    }
    // Constructor values are stored, so must be clean
    शून्य visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        जर (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        जर (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        जर (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, सत्य);
    }
    शून्य visit(Ast_गाठ_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    शून्य visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, असत्य);
        // We always clean, as we don't trust those pesky users.
        जर (!VN_IS(nodep->backp(), And)) घाला_Clean(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    शून्य visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->valuep());
    }
    शून्य visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    शून्य visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    शून्य visit(AstLoop* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य visit(Ast_गाठ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->exprsp());
        setClean(nodep, सत्य);  // generates a string, so not relevant
    }
    शून्य visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->nodesp());
    }
    शून्य visit(Ast_गाठ_CCall* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->argsp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensureCleanAndNext(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    शून्य visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सत्य);
    }
    शून्य visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, सत्य);
    }
    शून्य visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, सत्य);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    शून्य visit(Ast_गाठ* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Clean class functions

शून्य V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("clean", 0, dump_वृक्ष_Either_Level() >= 3);
}
