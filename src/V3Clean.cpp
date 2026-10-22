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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Clean.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clean state, as a visitor of each AstNode

తరగతి CleanVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstNode::user1()        -> CleanState.  For this node, 0==UNKNOWN
    //  AstNode::user2()        -> bool.  True indicates widthMin has been propagated
    //  AstNodeDType::user3()   -> AstNodeDType*.  Alternative node with C size
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;

    // TYPES
    గణన CleanState : uint8_t { CS_UNKNOWN, CS_CLEAN, CS_DIRTY };

    // STATE - for current visit position (use VL_RESTORER)
    స్థిరరాశి Ast_కణుపు_Module* m_modp = శూన్యసూచిక;

    // METHODS

    // Width resetting
    పూర్ణాంకం cpp_వెడల్పు(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (nodep->వెడల్పు() <= VL_IDATASIZE) {
            తిరిగి VL_IDATASIZE;
        } లేకపోతే ఒకవేళ (nodep->వెడల్పు() <= VL_QUADSIZE) {
            తిరిగి VL_QUADSIZE;
        } లేకపోతే {
            తిరిగి nodep->వెడల్పు_పదాలు() * VL_EDATASIZE;
        }
    }
    శూన్య సమితి_Cpp_వెడల్పు(Ast_కణుపు* nodep) {
        nodep->user2(నిజం);  // Don't resize it again
        Ast_కణుపు_DType* స్థిరరాశి old_dtypep = nodep->dtypep();
        స్థిరరాశి పూర్ణాంకం వెడల్పు = cpp_వెడల్పు(nodep);  // widthMin is unchanged
        ఒకవేళ (old_dtypep->వెడల్పు() != వెడల్పు) {
            // Since any given dtype's cppWidth() is the same, we can just
            // remember one conversion for each, and reuse it
            ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి new_dtypep = VN_CAST(old_dtypep->user3p(), కణుపు_DType)) {
                nodep->dtypep(new_dtypep);
            } లేకపోతే {
                nodep->dtype_Chg_వెడల్పు(వెడల్పు, nodep->వెడల్పు_Min());
                Ast_కణుపు_DType* స్థిరరాశి new_dtypep2 = nodep->dtypep();
                UASSERT_OBJ(new_dtypep2 != old_dtypep, nodep,
                            "Dtype didn't change when width changed");
                old_dtypep->user3p(new_dtypep2);  // Remember for next time
            }
        }
    }
    శూన్య compute_Cpp_వెడల్పు(Ast_కణుపు* nodep) {
        ఒకవేళ (!nodep->user2() && nodep->hasDType()) {
            ఒకవేళ (VN_IS(nodep, Var)  //
                || VN_IS(nodep, కణుపు_DType)  // Don't want to change variable widths!
                || VN_IS(nodep, కణుపు_Sel)  // Array selects should reflect variable widths
                || VN_IS(nodep, SFormatArg)  // Retain the logical argument type, not storage width
                || VN_IS(nodep->dtypep()->skipRefp(), AssocArrayDType)  // Or arrays
                || VN_IS(nodep->dtypep()->skipRefp(), WildcardArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), DynArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)
                || VN_IS(nodep->dtypep()->skipRefp(), వరుస_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), ప్రవాహం_DType)
                || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverCrossDType)
                || VN_IS(nodep->dtypep()->skipRefp(), CoverpointDType)
                || VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) {
            } లేకపోతే {
                స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి dtypep
                    = VN_CAST(nodep->dtypep()->skipRefp(), కణుపు_UOr_Struct_DType);
                ఒకవేళ (!dtypep || dtypep->packed()) సమితి_Cpp_వెడల్పు(nodep);
            }
        }
    }

    // Store the clean state in the userp on each node
    శూన్య సమితి_Clean_State(Ast_కణుపు* nodep, CleanState clean) { nodep->user1(clean); }
    CleanState getCleanState(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి static_cast<CleanState>(nodep->user1());
    }
    తార్కిక isClean(Ast_కణుపు* nodep) {
        స్థిరరాశి CleanState clstate = getCleanState(nodep);
        ఒకవేళ (clstate == CS_CLEAN) తిరిగి నిజం;
        ఒకవేళ (clstate == CS_DIRTY) తిరిగి అబద్ధం;
        nodep->v3fatalSrc("Unknown clean state on node: " + nodep->prettyTypeName());
        తిరిగి అబద్ధం;
    }
    శూన్య సమితి_Clean(Ast_కణుపు* nodep, తార్కిక isClean) {
        compute_Cpp_వెడల్పు(nodep);  // Just to be sure it's in widthMin
        స్థిరరాశి తార్కిక సంపూర్ణ_Uint
            = (nodep->వెడల్పు_Min() == VL_IDATASIZE || nodep->వెడల్పు_Min() == VL_QUADSIZE
               || (nodep->వెడల్పు_Min() % VL_EDATASIZE) == 0);
        సమితి_Clean_State(nodep, ((isClean || సంపూర్ణ_Uint) ? CS_CLEAN : CS_DIRTY));
    }

    // Operate on nodes
    శూన్య చొప్పించు_Clean(Ast_కణుపు_సమాసం* nodep) {  // We'll insert ABOVE passed node
        UINFO(4, "  NeedClean " << nodep);
        VNRelinker relinkHandle;
        nodep->unlink_Fr_వెనుక(&relinkHandle);
        //
        compute_Cpp_వెడల్పు(nodep);
        V3_సంఖ్య ముసుగు{nodep, cpp_వెడల్పు(nodep)};
        ముసుగు.సమితి_ముసుగు(nodep->వెడల్పు_Min());
        Ast_కణుపు* స్థిరరాశి cleanp
            = కొత్త AstAnd{nodep->fileline(), కొత్త AstConst{nodep->fileline(), ముసుగు}, nodep};
        cleanp->dtypeFrom(nodep);  // Otherwise the AND normally picks LHS
        relinkHandle.relink(cleanp);
    }
    శూన్య ensureClean(Ast_కణుపు_సమాసం* nodep) {
        compute_Cpp_వెడల్పు(nodep);
        ఒకవేళ (!isClean(nodep)) చొప్పించు_Clean(nodep);
    }
    శూన్య ensure_Clean_And_తదుపరి(Ast_కణుపు* nodep) {
        // Editing list, careful looping!
        కొరకు (Ast_కణుపు* argp = nodep; argp;) {
            Ast_కణుపు* స్థిరరాశి nextp = argp->nextp();
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_CAST(argp, కణుపు_సమాసం)) ensureClean(exprp);
            argp = nextp;
        }
    }

    // Base type handling methods
    శూన్య నిర్వాహ్యం_Biop(Ast_కణుపు_Biop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        ఒకవేళ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ఒకవేళ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        // no setClean.. must do it in each user routine.
    }
    శూన్య నిర్వాహ్యం_Triop(Ast_కణుపు_Triop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        ఒకవేళ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ఒకవేళ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ఒకవేళ (nodep->cleanThs()) ensureClean(nodep->thsp());
        // no setClean.. must do it in each user routine.
    }
    శూన్య నిర్వాహ్యం_Quadop(Ast_కణుపు_Quadop* nodep) {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        ఒకవేళ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        ఒకవేళ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
        ఒకవేళ (nodep->cleanThs()) ensureClean(nodep->thsp());
        ఒకవేళ (nodep->cleanFhs()) ensureClean(nodep->fhsp());
        // no setClean.. must do it in each user routine.
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        ఒకవేళ (nodep->cleanLhs()) ensureClean(nodep->lhsp());
        సమితి_Clean(nodep, nodep->clean_బహిర్గతం());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        నిర్వాహ్యం_Biop(nodep);
        సమితి_Clean(nodep, nodep->clean_బహిర్గతం());
    }
    శూన్య సందర్శించు(AstAnd* nodep) override {
        నిర్వాహ్యం_Biop(nodep);
        సమితి_Clean(nodep, isClean(nodep->lhsp()) || isClean(nodep->rhsp()));
    }
    శూన్య సందర్శించు(AstXor* nodep) override {
        నిర్వాహ్యం_Biop(nodep);
        సమితి_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    శూన్య సందర్శించు(AstOr* nodep) override {
        నిర్వాహ్యం_Biop(nodep);
        సమితి_Clean(nodep, isClean(nodep->lhsp()) && isClean(nodep->rhsp()));
    }
    శూన్య సందర్శించు(Ast_కణుపు_Quadop* nodep) override {
        నిర్వాహ్యం_Quadop(nodep);
        సమితి_Clean(nodep, nodep->clean_బహిర్గతం());
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        సమితి_Clean(nodep, isClean(nodep->resultp()));
    }
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        సమితి_Clean(nodep, nodep->clean_బహిర్గతం());
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        ఒకవేళ (nodep->cleanRhs()) ensureClean(nodep->rhsp());
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {  //
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstCNew* nodep) override {
        iterateChildren(nodep);
        సమితి_Clean(nodep, నిజం);
    }
    // Constructor values are stored, so must be clean
    శూన్య సందర్శించు(AstConsAssoc* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->defaultp()) ensureClean(nodep->defaultp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstConsDynArray* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->defaultp()) ensureClean(nodep->defaultp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstConsWildcard* nodep) override {
        iterateChildren(nodep);
        ఒకవేళ (nodep->defaultp()) ensureClean(nodep->defaultp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cons_అంశం* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->valuep());
    }
    శూన్య సందర్శించు(Ast_సరిపోలిక_Masked* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        నిర్వాహ్యం_Biop(nodep);
        సమితి_Clean(nodep, nodep->clean_బహిర్గతం());
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
        సమితి_Clean(nodep, అబద్ధం);
        // We always clean, as we don't trust those pesky users.
        ఒకవేళ (!VN_IS(nodep->backp(), And)) చొప్పించు_Clean(nodep);
        ensure_Clean_And_తదుపరి(nodep->nodesp());
    }
    శూన్య సందర్శించు(AstTraceDecl* nodep) override {}  // Nothing to do here
    శూన్య సందర్శించు(AstTraceInc* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->valuep());
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {
        // No cleaning, or would loose pointer to enum
        iterateChildren(nodep);
    }

    // Control flow operators
    శూన్య సందర్శించు(AstCond* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
        సమితి_Clean(nodep, isClean(nodep->thenp()) && isClean(nodep->elsep()));
    }
    శూన్య సందర్శించు(AstLoop* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->condp());
    }
    శూన్య సందర్శించు(AstSFormatArg* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->exprp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->exprsp());
        సమితి_Clean(nodep, నిజం);  // generates a string, so not relevant
    }
    శూన్య సందర్శించు(AstCStmt* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->nodesp());
    }
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->nodesp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->argsp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        ensure_Clean_And_తదుపరి(nodep->pinsp());
        సమితి_Clean(nodep, nodep->clean_బహిర్గతం());
    }
    శూన్య సందర్శించు(AstWith* nodep) override {
        iterateChildren(nodep);
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstCReturn* nodep) override {
        iterateChildren(nodep);
        ensureClean(nodep->lhsp());
        సమితి_Clean(nodep, నిజం);
    }
    శూన్య సందర్శించు(AstIntfRef* nodep) override {
        iterateChildren(nodep);
        సమితి_Clean(nodep, నిజం);  // generates a string, so not relevant
    }

    //--------------------
    // Default: Just iterate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        iterateChildren(nodep);
        compute_Cpp_వెడల్పు(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CleanVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CleanVisitor() override = అప్రమేయ;
};

//######################################################################
// Clean class functions

శూన్య V3Clean::cleanAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CleanVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("clean", 0, dump_వృక్షం_Either_Level() >= 3);
}
