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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;

    // TYPES
    गणना CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;

    // METHODS

    // Width resetting
    पूर्णाङ्क cpp_विस्तारः(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (nodep->विस्तारः() <= VL_IDATASIZE) {
            प्रत्यागम VL_IDATASIZE;
        } अन्यथा यदि (nodep->विस्तारः() <= VL_QUADSIZE) {
            प्रत्यागम VL_QUADSIZE;
        } अन्यथा {
            प्रत्यागम nodep->विस्तारः_शब्दाः() * VL_EDATASIZE;
        }
    }
    शून्य समुच्चयः_Cpp_विस्तारः(Ast_ग्रन्थिः* nodep) {
        nodep->user2(सत्य);  // Don't resize it again
        Ast_ग्रन्थिः_DType* ध्रुव old_dtypep = nodep->dtypep();
        ध्रुव पूर्णाङ्क विस्तारः = cpp_विस्तारः(nodep);  // widthMin is unchanged
        यदि (old_dtypep->विस्तारः() != विस्तारः) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            यदि (Ast_ग्रन्थिः_DType* ध्रुव new_dtypep = VN_CAST(old_dtypep->user3p(), ग्रन्थिः_DType)) {
                nodep->dtypep(new_dtypep);
            } अन्यथा {
                nodep->dtype_Chg_विस्तारः(विस्तारः, nodep->विस्तारः_Min());
                Ast_ग्रन्थिः_DType* ध्रुव new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    शून्य compute_Cpp_विस्तारः(Ast_ग्रन्थिः* nodep) {
        यदि (!nodep->user2() && nodep->hasDType()) {
            यदि (VN_IS(nodep, Var)  //
                || VN_IS(nodep, ग्रन्थिः_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, ग्रन्थिः_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), QueueDType)
                || VN_IS(nodep->dtypep()->skipRefp(), प्रवाहः_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } अन्यथा {
                ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), ग्रन्थिः_UOr_Struct_DType);
                यदि (!dtypep || dtypep->packed()) समुच्चयः_Cpp_विस्तारः(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    शून्य समुच्चयः_Clean_State(Ast_ग्रन्थिः* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम static_cast<CleanState>(nodep->user1());
    }
    तार्किक isClean(Ast_ग्रन्थिः* nodep) {
        ध्रुव CleanState clstate = getCleanState(nodep);
        यदि (clstate == CS_CLEAN) प्रत्यागम सत्य;
        यदि (clstate == CS_DIRTY) प्रत्यागम असत्य;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        प्रत्यागम असत्य;
    }
    शून्य समुच्चयः_Clean(Ast_ग्रन्थिः* nodep, तार्किक isClean) {
        compute_Cpp_विस्तारः(nodep);  // Just to be sure it's in widthMin
        ध्रुव तार्किक सम्पूर्णम्_Uint
            = (nodep->विस्तारः_Min() == VL_IDATASIZE || nodep->विस्तारः_Min() == VL_QUADSIZE
               || (nodep->विस्तारः_Min() % VL_EDATASIZE) == 0);
        समुच्चयः_Clean_State(nodep, ((isClean || सम्पूर्णम्_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    शून्य निवेशय_Clean(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_पृष्ठभागः(&relinkHandle);
        //
        compute_Cpp_विस्तारः(nodep);
        V3_सङ्ख्या आवरणम्{nodep, cpp_विस्तारः(nodep)};
        आवरणम्.समुच्चयः_आवरणम्(nodep->विस्तारः_Min());
        Ast_ग्रन्थिः* ध्रुव cleanp
            = नूतन AstAnd{nodep->fileline(), नूतन AstConst{nodep->fileline(), आवरणम्}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    शून्य ensureClean(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        compute_Cpp_विस्तारः(nodep);
        यदि (!isClean(nodep)) निवेशय_Clean(nodep);
    }
    शून्य ensure_Clean_And_अग्र(Ast_ग्रन्थिः* nodep) {
        // Editing list, careful looping!
        प्रति (Ast_ग्रन्थिः* argp = nodep; argp;) {
            Ast_ग्रन्थिः* ध्रुव nextp = argp->nextp();
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(argp, ग्रन्थिः_व्यञ्जकम्)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    शून्य सङ्कार्यम्_Biop(Ast_ग्रन्थिः_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य सङ्कार्यम्_Triop(Ast_ग्रन्थिः_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        यदि (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    शून्य सङ्कार्यम्_Quadop(Ast_ग्रन्थिः_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        यदि (nodep->cleanThs()) ensureClean(nodep->thsp());
        यदि (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        यदि (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        समुच्चयः_Clean(nodep, nodep->clean_बहिर्गमनम्());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        सङ्कार्यम्_Biop(nodep);
        समुच्चयः_Clean(nodep, nodep->clean_बहिर्गमनम्());
    }
    शून्य दर्शय(AstAnd* nodep) override {
        सङ्कार्यम्_Biop(nodep);
        समुच्चयः_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    शून्य दर्शय(AstXor* nodep) override {
        सङ्कार्यम्_Biop(nodep);
        समुच्चयः_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    शून्य दर्शय(AstOr* nodep) override {
        सङ्कार्यम्_Biop(nodep);
        समुच्चयः_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Quadop* nodep) override {
        सङ्कार्यम्_Quadop(nodep);
        समुच्चयः_Clean(nodep, nodep->clean_बहिर्गमनम्());
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        समुच्चयः_Clean(nodep, isClean(nodep->resultp()));
    }
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        समुच्चयः_Clean(nodep, nodep->clean_बहिर्गमनम्());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        यदि (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    शून्य दर्शय(AstScopeName* nodep) override {  //
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstCNew* nodep) override {
        iterateChildren(nodep);
        समुच्चयः_Clean(nodep, सत्य);
    }
    // Constructor values are stored, so must be clean
    शून्य दर्शय(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) ensureClean(nodep->defaultp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) ensureClean(nodep->defaultp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        यदि (nodep->defaultp()) ensureClean(nodep->defaultp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cons_Item* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    शून्य दर्शय(Ast_मेलनम्_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstSel* nodep) override {
        सङ्कार्यम्_Biop(nodep);
        समुच्चयः_Clean(nodep, nodep->clean_बहिर्गमनम्());
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
        समुच्चयः_Clean(nodep, असत्य);
        // We always clean, as we don't trust those pesky users.
        यदि (!VN_IS(nodep->backp(), And)) निवेशय_Clean(nodep);
        ensure_Clean_And_अग्र(nodep->nodesp());
    }
    शून्य दर्शय(AstTraceDecl* nodep) override {}  // Nothing to do here
    शून्य दर्शय(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->valuep());
    }
    शून्य दर्शय(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    शून्य दर्शय(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        समुच्चयः_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    शून्य दर्शय(AstLoop* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    शून्य दर्शय(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->exprp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->exprsp());
        समुच्चयः_Clean(nodep, सत्य);  // generates a string, so not relevant
    }
    शून्य दर्शय(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->nodesp());
    }
    शून्य दर्शय(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->nodesp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->argsp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_अग्र(nodep->pinsp());
        समुच्चयः_Clean(nodep, nodep->clean_बहिर्गमनम्());
    }
    शून्य दर्शय(AstWith* nodep) override {
        iterateChildren(nodep);
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        समुच्चयः_Clean(nodep, सत्य);
    }
    शून्य दर्शय(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        समुच्चयः_Clean(nodep, सत्य);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_विस्तारः(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = स्वभाव;
};

//######################################################################
// Clean class functions

शून्य V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("clean", 0, dump_वृक्षः_Either_Level() >= 3);
}
