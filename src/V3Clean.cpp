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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

श्रेणी CleanVisitor final : खुला VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    स्थिर VNUser3InUse m_inuser3;

    // TYPES
    क्रमागत CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    स्थिर Ast_गाँठ_Module* m_modp = nullptr;

    // METHODS

    // Width resetting
    पूर्णांक cppWidth(स्थिर Ast_गाँठ* nodep) {
        अगर (nodep->width() <= VL_IDATASIZE) {
            वापस VL_IDATASIZE;
        } अन्यथा अगर (nodep->width() <= VL_QUADSIZE) {
            वापस VL_QUADSIZE;
        } अन्यथा {
            वापस nodep->width_शब्दगण() * VL_EDATASIZE;
        }
    }
    व्योम setCppWidth(Ast_गाँठ* nodep) {
        nodep->user2(true);  // Don't resize it again
        Ast_गाँठ_DType* स्थिर old_dtypep = nodep->dtypep();
        स्थिर पूर्णांक width = cppWidth(nodep);  // widthMin is unchanged
        अगर (old_dtypep->width() != width) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            अगर (Ast_गाँठ_DType* स्थिर new_dtypep = VN_CAST(old_dtypep->user3p(), गाँठ_DType)) {
                nodep->dtypep(new_dtypep);
            } अन्यथा {
                nodep->dtypeChgWidth(width, nodep->widthMin());
                Ast_गाँठ_DType* स्थिर new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    व्योम computeCppWidth(Ast_गाँठ* nodep) {
        अगर (!nodep->user2() && nodep->hasDType()) {
            अगर (VN_IS(nodep, Var)  //
                || VN_IS(nodep, गाँठ_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, गाँठ_Sel)  // Array selects should reflect variable widths
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
                स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), गाँठ_UOr_Struct_DType);
                अगर (!dtypep || dtypep->packed()) setCppWidth(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    व्योम setCleanState(Ast_गाँठ* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(स्थिर Ast_गाँठ* nodep) {
        वापस static_cast<CleanState>(nodep->user1());
    }
    bool isClean(Ast_गाँठ* nodep) {
        स्थिर CleanState clstate = getCleanState(nodep);
        अगर (clstate == CS_CLEAN) वापस true;
        अगर (clstate == CS_DIRTY) वापस false;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        वापस false;
    }
    व्योम setClean(Ast_गाँठ* nodep, bool isClean) {
        computeCppWidth(nodep);  // Just to be sure it's in widthMin
        स्थिर bool wholeUint
            = (nodep->widthMin() == VL_IDATASIZE || nodep->widthMin() == VL_QUADSIZE
               || (nodep->widthMin() % VL_EDATASIZE) == 0);
        setCleanState(nodep, ((isClean || wholeUint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    व्योम जोड़ो_Clean(Ast_गाँठ_व्यंजक* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        //
        computeCppWidth(nodep);
        V3_संख्या mask{nodep, cppWidth(nodep)};
        mask.setMask(nodep->widthMin());
        Ast_गाँठ* स्थिर cleanp
            = नया AstAnd{nodep->fileline(), नया AstConst{nodep->fileline(), mask}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    व्योम ensureClean(Ast_गाँठ_व्यंजक* nodep) {
        computeCppWidth(nodep);
        अगर (!isClean(nodep)) जोड़ो_Clean(nodep);
    }
    व्योम ensure_Clean_And_अग्र(Ast_गाँठ* nodep) {
        // Editing list, careful looping!
        क्रम (Ast_गाँठ* argp = nodep; argp;) {
            Ast_गाँठ* स्थिर nextp = argp->nextp();
            अगर (Ast_गाँठ_व्यंजक* स्थिर exprp = VN_CAST(argp, गाँठ_व्यंजक)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    व्योम operandBiop(Ast_गाँठ_Biop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        अगर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        अगर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    व्योम operandTriop(Ast_गाँठ_Triop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        अगर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        अगर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        अगर (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    व्योम operandQuadop(Ast_गाँठ_Quadop* nodep) {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        अगर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        अगर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        अगर (nodep->cleanThs()) ensureClean(nodep->thsp());
        अगर (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Uniop* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        अगर (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        setClean(nodep, nodep->cleanOut());
    }
    व्योम visit(Ast_गाँठ_Biop* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    व्योम visit(AstAnd* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    व्योम visit(AstXor* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    व्योम visit(AstOr* nodep) override {
        operandBiop(nodep);
        setClean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    व्योम visit(Ast_गाँठ_Quadop* nodep) override {
        operandQuadop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, isClean(nodep->resultp()));
    }
    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        अगर (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    व्योम visit(AstScopeName* nodep) override {  //
        setClean(nodep, true);
    }
    व्योम visit(AstCNew* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, true);
    }
    // Constructor values are stored, so must be clean
    व्योम visit(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, true);
    }
    व्योम visit(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, true);
    }
    व्योम visit(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        अगर (nodep->defaultp()) ensureClean(nodep->defaultp());
        setClean(nodep, true);
    }
    व्योम visit(Ast_गाँठ_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    व्योम visit(AstMatchMasked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, true);
    }
    व्योम visit(AstSel* nodep) override {
        operandBiop(nodep);
        setClean(nodep, nodep->cleanOut());
    }
    व्योम visit(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
        setClean(nodep, false);
        // We always clean, as we don't trust those pesky users.
        अगर (!VN_IS(nodep->backp(), And)) जोड़ो_Clean(nodep);
        ensure_Clean_And_अग्र(nodep->nodesp());
    }
    व्योम visit(AstTraceDecl* nodep) override {}  // Nothing to do here
    व्योम visit(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->valuep());
    }
    व्योम visit(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    व्योम visit(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    व्योम visit(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        setClean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    व्योम visit(AstLoop* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    व्योम visit(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->exprp());
        setClean(nodep, true);
    }
    व्योम visit(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->exprsp());
        setClean(nodep, true);  // generates a string, so not relevant
    }
    व्योम visit(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->nodesp());
    }
    व्योम visit(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->nodesp());
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->argsp());
        setClean(nodep, true);
    }
    व्योम visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->pinsp());
        setClean(nodep, nodep->cleanOut());
    }
    व्योम visit(AstWith* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, true);
    }
    व्योम visit(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        setClean(nodep, true);
    }
    व्योम visit(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        setClean(nodep, true);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    व्योम visit(Ast_गाँठ* nodep) override {
        iterateChildren(nodep);
        computeCppWidth(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = शेष;
};

//######################################################################
// Clean class functions

व्योम V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("clean", 0, dump_वृक्ष_Either_Level() >= 3);
}
