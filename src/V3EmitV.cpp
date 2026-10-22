// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Verilog from tree
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#समावेश "V3PchAstMT.h"

#समावेश "V3EmitV.h"

#समावेश "V3EmitCBase.h"

#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

श्रेणी EmitVBaseVisitorConst VL_NOT_FINAL : खुला VNVisitorConst {
    // STATE - across all visitors
    स्थिर bool m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_वृक्ष* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    bool m_suppressSemi = false;  // Non-statement, don't print ;
    bool m_suppressVarSemi = false;  // Suppress emitting semicolon for AstVars
    bool m_suppressSampled = false;  // Suppress emitting sampled in assertion properties
    bool m_arrayPost = false;  // Print array information that goes after identifier (vs after)
    bool m_prefixed = true;  // Whether constants need to be prefixed
    std::deque<Ast_गाँठ_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, माप_प्रकार> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    भव व्योम माला_दो(स्थिर string& str) = 0;
    भव व्योम putbs(स्थिर string& str) = 0;
    भव व्योम putfs(Ast_गाँठ* nodep, स्थिर string& str) = 0;  // Fileline and node %% mark
    भव व्योम putqs(Ast_गाँठ* nodep, स्थिर string& str) = 0;  // Fileline quiet w/o %% mark
    भव व्योम putsNoTracking(स्थिर string& str) = 0;
    भव व्योम putsQuoted(स्थिर string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    व्योम iterateAndCommaConstNull(Ast_गाँठ* nodep) {
        क्रम (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            अगर (nodep->nextp()) माला_दो(", ");
        }
    }
    व्योम emitPacked() {
        क्रम (Ast_गाँठ_Array_DType* packedp : m_packedps) {
            माला_दो(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.सफा();
    }
    व्योम emitNodesWithText(Ast_गाँठ* nodesp, bool tracking, स्थिर std::string& separator) {
        क्रम (Ast_गाँठ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstText* स्थिर textp = VN_CAST(nodep, Text)) {
                अगर (tracking) {
                    माला_दो(textp->text());
                } अन्यथा {
                    putsNoTracking(textp->text());
                }
            } अन्यथा {
                iterateConst(nodep);
            }
            अगर (nodep->nextp()) माला_दो(separator);
        }
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override { iterate_And_अग्र_Const_Null(nodep->modulesp()); }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    व्योम visit(AstPort* nodep) override {}
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        माला_दो(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    व्योम visit(AstGenBlock* nodep) override {
        स्थिर std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        माला_दो("end" + name + '\n');
    }
    व्योम visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो(")\n");
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    व्योम visit(AstGenCaseItem* nodep) override {
        अगर (nodep->condsp()) {
            iterate_And_अग्र_Const_Null(nodep->condsp());
        } अन्यथा {
            putbs("default");
        }
        iterate_And_अग्र_Const_Null(nodep->itemsp());
    }
    व्योम visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = true;
            iterate_And_अग्र_Const_Null(nodep->initsp());
            माला_दो(";");
            iterate_And_अग्र_Const_Null(nodep->condp());
            माला_दो(";");
            iterate_And_अग्र_Const_Null(nodep->incsp());
        }
        माला_दो(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    व्योम visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        माला_दो("/* generate */ if (");
        iterate_And_अग्र_Const_Null(nodep->condp());
        माला_दो(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->thensp());
        अगर (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_अग्र_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    व्योम visit(AstBegin* nodep) override {
        अगर (nodep->name() == "") {
            putbs("begin\n");
        } अन्यथा {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        माला_दो("end\n");
    }
    व्योम visit(AstFork* nodep) override {
        अगर (nodep->name() == "") {
            putbs("fork\n");
        } अन्यथा {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        माला_दो(nodep->joinType().verilogKwd());
        माला_दो("\n");
    }
    व्योम visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    व्योम visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    व्योम visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstAlways* nodep) override {
        अगर (स्थिर AstAssignW* स्थिर ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            अगर (!ap->nextp()) {
                putfs(nodep, "assign ");
                अगर (Ast_गाँठ* स्थिर tcp = ap->timingControlp()) {
                    iterate_And_अग्र_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_अग्र_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_अग्र_Const_Null(ap->rhsp());
                अगर (!m_suppressSemi) माला_दो(";\n");
                वापस;
            }
        }
        putfs(nodep, "always ");
        अगर (m_sentreep) {
            iterate_And_अग्र_Const_Null(m_sentreep);
        }  // In active
        अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    व्योम visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    व्योम visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (VN_IS(nodep, AssignForce)) माला_दो("force ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        अगर (!m_suppressSemi) माला_दो(";\n");
    }
    व्योम visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        क्रम (Ast_गाँठ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        अगर (!m_suppressSemi) माला_दो(";\n");
    }
    व्योम visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        अगर (!m_suppressSemi) माला_दो(";\n");
    }
    व्योम visit(AstRelease* nodep) override {
        माला_दो("release ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        अगर (!m_suppressSemi) माला_दो(";\n");
    }
    व्योम visit(AstBreak*) override {
        putbs("break");
        अगर (!m_suppressSemi) माला_दो(";\n");
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        क्रम (Ast_गाँठ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            अगर (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        माला_दो(")");
    }
    व्योम visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        अगर (nodep->edgeType() != VEdgeType::ET_CHANGED) माला_दो(nodep->edgeType().verilogKwd());
        अगर (nodep->sensp()) माला_दो(" ");
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCReset* /*nodep*/) override { माला_दो("/*CRESET*/"); }
    व्योम visit(AstCase* nodep) override {
        putfs(nodep, "");
        अगर (nodep->priorityPragma()) माला_दो("priority ");
        अगर (nodep->uniquePragma()) माला_दो("unique ");
        अगर (nodep->unique0Pragma()) माला_दो("unique0 ");
        माला_दो(nodep->verilogKwd());
        माला_दो(" (");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो(")\n");
        अगर (nodep->fullPragma() || nodep->parallelPragma()) {
            माला_दो(" // synopsys");
            अगर (nodep->fullPragma()) माला_दो(" full_case");
            अगर (nodep->parallelPragma()) माला_दो(" parallel_case");
        }
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    व्योम visit(AstCaseItem* nodep) override {
        अगर (nodep->condsp()) {
            iterate_And_अग्र_Const_Null(nodep->condsp());
        } अन्यथा {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    व्योम visit(AstComment* nodep) override {
        माला_दो("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstContinue*) override {
        putbs("continue");
        अगर (!m_suppressSemi) माला_दो(";\n");
    }
    व्योम visit(Ast_गाँठ_Cover_Decl*) override {}  // N/A
    व्योम visit(AstCoverInc*) override {}  // N/A
    व्योम visit(AstCoverToggle*) override {}  // N/A

    व्योम visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        अगर (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    व्योम visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        अगर (nodep->binsp() || nodep->optionsp()) {
            माला_दो(" {\n");
            iterate_And_अग्र_Const_Null(nodep->optionsp());
            iterate_And_अग्र_Const_Null(nodep->binsp());
            माला_दो("}");
        }
        माला_दो(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    व्योम emitCoverageOption(Ast_गाँठ* nodep, bool typeOption, VCoverOptionType optType,
                            Ast_गाँठ_व्यंजक* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        माला_दो(";\n");
    }
    व्योम visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    व्योम visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    व्योम visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        माला_दो(nodep->name());
        अगर (nodep->isArray()) {
            माला_दो("[");
            अगर (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            माला_दो("]");
        }
        अगर (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            माला_दो(" = default");
        } अन्यथा अगर (nodep->transp()) {
            माला_दो(" = ");
            क्रम (Ast_गाँठ* setp = nodep->transp(); setp; setp = setp->nextp()) {
                अगर (setp != nodep->transp()) माला_दो(", ");
                iterateConst(setp);
            }
        } अन्यथा अगर (VN_IS(nodep->rangesp(), CoverWith)) {
            माला_दो(" = ");
            iterateConst(nodep->rangesp());
        } अन्यथा अगर (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            माला_दो(" = {");
            क्रम (Ast_गाँठ* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                अगर (rangep != nodep->rangesp()) माला_दो(", ");
                iterateConst(rangep);
            }
            माला_दो("}");
        }
        अगर (nodep->iffp()) {
            माला_दो(" iff (");
            iterateConst(nodep->iffp());
            माला_दो(")");
        }
        माला_दो(";\n");
    }
    व्योम visit(AstCoverWith* nodep) override {
        स्थिर bool rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        अगर (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        अगर (rangeList) माला_दो("}");
        माला_दो(" with (");
        iterateConst(nodep->filterp());
        माला_दो(")");
    }
    व्योम visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        अगर (!nodep->name().empty()) माला_दो("." + nodep->name());
        माला_दो(")");
        अगर (nodep->rangesp()) {
            माला_दो(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            माला_दो("}");
        }
    }
    व्योम visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        अगर (nodep->iffp()) {
            माला_दो(" iff (");
            iterateConst(nodep->iffp());
            माला_दो(")");
        }
        माला_दो(";\n");
    }
    व्योम visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    व्योम visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        माला_दो(")");
    }
    व्योम visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        अगर (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        माला_दो(")");
        अगर (nodep->countp()) {
            माला_दो(" matches ");
            iterateConst(nodep->countp());
            माला_दो(")");
        }
    }
    व्योम visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    व्योम visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        क्रम (Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            अगर (itemp != nodep->itemsp()) माला_दो(", ");
            iterateConst(itemp);
        }
        अगर (nodep->iffp()) {
            माला_दो(" iff (");
            iterateConst(nodep->iffp());
            माला_दो(")");
        }
        अगर (nodep->binsp() || nodep->optionsp()) {
            माला_दो(" {\n");
            iterate_And_अग्र_Const_Null(nodep->optionsp());
            iterate_And_अग्र_Const_Null(nodep->binsp());
            माला_दो("}\n");
        } अन्यथा {
            माला_दो(";\n");
        }
    }
    व्योम visit(AstCoverTransSet* nodep) override {
        माला_दो("(");
        क्रम (Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            अगर (itemp != nodep->itemsp()) माला_दो(" => ");
            iterateConst(itemp);
        }
        माला_दो(")");
    }
    व्योम visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    व्योम visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        अगर (AstConst* स्थिर lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->अंक().toString());
        } अन्यथा {
            माला_दो("string'(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            माला_दो(")");
        }
    }
    व्योम visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        माला_दो(")");
    }
    व्योम visit(Ast_मूल्य_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        माला_दो(")");
    }
    व्योम visit_गाँठ_Display(Ast_गाँठ* nodep, Ast_गाँठ* fileOrStrgp, स्थिर string& text,
                          Ast_गाँठ* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        अगर (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        क्रम (Ast_गाँठ* expp = exprsp; expp; expp = expp->nextp()) {
            माला_दो(", ");
            iterateConstNull(expp);
        }
        माला_दो(");\n");
    }
    व्योम visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    व्योम visit(AstDisplay* nodep) override {
        visit_गाँठ_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    व्योम visit(AstElabDisplay* nodep) override {
        visit_गाँठ_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    व्योम visit(AstFScanF* nodep) override {
        visit_गाँठ_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    व्योम visit(AstSScanF* nodep) override {
        visit_गाँठ_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    व्योम visit(AstSFormat* nodep) override {
        visit_गाँठ_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    व्योम visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    व्योम visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    व्योम visit(AstSFormatF* nodep) override {
        visit_गाँठ_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    व्योम visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_अग्र_Const_Null(nodep->modep());
        माला_दो(");\n");
    }
    व्योम visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->filenamep());
        माला_दो(");\n");
    }
    व्योम visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        अगर (nodep->filep()) iterate_And_अग्र_Const_Null(nodep->filep());
        माला_दो(");\n");
    }
    व्योम visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        अगर (nodep->filep()) iterate_And_अग्र_Const_Null(nodep->filep());
        माला_दो(");\n");
    }
    व्योम visit(AstJumpBlock* nodep) override {
        // Allocate label number
        स्थिर माप_प्रकार n = m_labelNumbers.आकार();
        स्थिर bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        माला_दो("end\n");
    }
    व्योम visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        स्थिर स्वतः it = m_labelNumbers.find(nodep->blockp());
        स्थिर std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    व्योम visit(Ast_गाँठ_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        अगर (nodep->filenamep()) iterate_And_अग्र_Const_Null(nodep->filenamep());
        putbs(", ");
        अगर (nodep->memp()) iterate_And_अग्र_Const_Null(nodep->memp());
        अगर (nodep->lsbp()) {
            putbs(", ");
            iterate_And_अग्र_Const_Null(nodep->lsbp());
        }
        अगर (nodep->msbp()) {
            putbs(", ");
            iterate_And_अग्र_Const_Null(nodep->msbp());
        }
        माला_दो(");\n");
    }
    व्योम visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->exprsp());
        माला_दो(");\n");
    }
    व्योम visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_अग्र_Const_Null(nodep->countp());
        माला_दो(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    व्योम visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        अगर (AstLoopTest* स्थिर testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            माला_दो(") begin\n");
            iterate_And_अग्र_Const_Null(testp->nextp());
            माला_दो("end\n");
            वापस;
        }
        // Special case when the AstLoopTest is last for output readability
        अगर (Ast_गाँठ* lastp = nodep->stmtsp()) {
            जबतक (Ast_गाँठ* स्थिर nextp = lastp->nextp()) lastp = nextp;
            अगर (AstLoopTest* स्थिर testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                क्रम (Ast_गाँठ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                माला_दो("end while (");
                iterateConst(testp->condp());
                माला_दो(")\n");
                वापस;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        iterate_And_अग्र_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    व्योम visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_अग्र_Const_Null(nodep->condp());
        माला_दो(")) break;\n");
    }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        putfs(nodep, "");
        अगर (स्थिर AstIf* स्थिर ifp = VN_CAST(nodep, If)) {
            अगर (ifp->priorityPragma()) माला_दो("priority ");
            अगर (ifp->uniquePragma()) माला_दो("unique ");
            अगर (ifp->unique0Pragma()) माला_दो("unique0 ");
        }
        माला_दो("if (");
        iterate_And_अग्र_Const_Null(nodep->condp());
        माला_दो(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->thensp());
        अगर (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_अग्र_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    व्योम visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        अगर (nodep->ticksp() || nodep->sentreep()) {
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->ticksp());
            अगर (nodep->sentreep()) {
                माला_दो(", ");
                iterate_And_अग्र_Const_Null(nodep->sentreep());
            }
        }
        माला_दो(")");
    }
    व्योम visit(AstSampled* nodep) override {
        अगर (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_अग्र_Const_Null(nodep->exprp());
            माला_दो(")");
        }
    }
    व्योम visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो(")");
    }
    व्योम visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        अगर (nodep->sentreep()) {
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        माला_दो(")");
    }
    व्योम visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        अगर (nodep->sentreep()) {
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        माला_दो(")");
    }
    व्योम visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो(")");
    }
    व्योम visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो(")");
    }
    व्योम visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        अगर (nodep->sentreep()) {
            माला_दो(", ");
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        माला_दो(")");
    }
    व्योम visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        माला_दो(")");
    }
    व्योम visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(";\n");
    }
    व्योम visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        माला_दो(";\n");
    }
    व्योम visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    व्योम visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        iterateConst(nodep->exprp());
        माला_दो(";\n");
    }

    // Nodes involing AstText
    व्योम visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    व्योम visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        माला_दो(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), true, nodep->separator());
        माला_दो(nodep->suffix());
    }
    व्योम visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), true, "");
        माला_दो(");\n");
    }
    व्योम visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), true, "");
        माला_दो(")");
    }
    व्योम visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), false, "");
        माला_दो(");\n");
    }
    व्योम visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), false, "");
        माला_दो(")");
    }

    व्योम visit(AstScopeName* nodep) override {}
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_अग्र_Const_Null(nodep->resultp());
        माला_दो(");\n");
    }

    व्योम visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        माला_दो(")");
        iterateConstNull(nodep->withp());
    }
    व्योम visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        माला_दो(")");
    }

    // Operators
    भव व्योम emitVerilogFormat(Ast_गाँठ* nodep, स्थिर string& format, Ast_गाँठ* lhsp = nullptr,
                                   Ast_गाँठ* स्थिर rhsp = nullptr, Ast_गाँठ* thsp = nullptr,
                                   Ast_गाँठ* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        bool inPct = false;
        putbs("");
        क्रम (स्थिर अक्षर c : format) {
            अगर (!inPct && c == '%') {
                inPct = true;
            } अन्यथा अगर (!inPct) {  // Normal text
                string s;
                s += c;
                माला_दो(s);
            } अन्यथा {  // Format character
                inPct = false;
                चयन (c) {
                हाल '%': माला_दो("%"); अवरोध;
                हाल 'f': putfs(nodep, ""); अवरोध;
                हाल 'k': putbs(""); अवरोध;
                हाल 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(lhsp);
                    अवरोध;
                }
                हाल 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(rhsp);
                    अवरोध;
                }
                हाल 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(thsp);
                    अवरोध;
                }
                हाल 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(fhsp);
                    अवरोध;
                }
                हाल 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(nodep->dtypep());
                    अवरोध;
                }
                शेष: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); अवरोध;
                }
            }
        }
    }

    व्योम visit(Ast_गाँठ_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    व्योम visit(Ast_गाँठ_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    व्योम visit(Ast_गाँठ_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    व्योम visit(Ast_गाँठ_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    व्योम visit(Ast_गाँठ_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    व्योम visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो(".");
        माला_दो(nodep->prettyName());
    }
    व्योम visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो(".");
        माला_दो(nodep->prettyName());
    }
    व्योम visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        अगर (nodep->dimp()) {
            putbs(", ");
            iterate_And_अग्र_Const_Null(nodep->dimp());
        }
        माला_दो(")");
    }
    व्योम visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        क्रम (स्थिर AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            अगर (itemp != nodep->itemsp()) putbs(", ");
            माला_दो(cvtToStr(itemp->सूचकांक_2()));
            माला_दो(":");
            iterateConst(itemp->valuep());
        }
        माला_दो("}");
    }
    व्योम visit(AstCond* nodep) override {
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_अग्र_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_अग्र_Const_Null(nodep->elsep());
        माला_दो(")");
    }
    व्योम visit(AstInsideRange* nodep) override {
        माला_दो("[");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        माला_दो(":");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        माला_दो("]");
    }
    व्योम visit(AstRange* nodep) override {
        माला_दो("[");
        अगर (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            माला_दो(cvtToStr(nodep->बायाँ_Const()));
            माला_दो(":");
            माला_दो(cvtToStr(nodep->दायाँ_Const()));
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->leftp());
            माला_दो(":");
            iterate_And_अग्र_Const_Null(nodep->rightp());
        }
        माला_दो("]");
    }
    व्योम visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    व्योम visit(AstSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        पूर्णांक offset = 0;
        Ast_गाँठ_DType* स्थिर dtypep = nodep->fromp()->dtypep();
        अगर (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* स्थिर basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        माला_दो("[");
        अगर (VN_IS(nodep->lsbp(), Const)) {
            अगर (nodep->widthConst() == 1) {
                माला_दो(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } अन्यथा {
                माला_दो(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                माला_दो(":");
                माला_दो(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->lsbp());
            अगर (offset != 0) {
                माला_दो(" + ");
                माला_दो(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            माला_दो(cvtToStr(nodep->widthConst()));
        }
        माला_दो("]");
    }
    व्योम visit(AstSliceSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        माला_दो(cvtToStr(nodep->declRange()));
    }
    व्योम visit(AstThisRef* /*nodep*/) override { माला_दो("this"); }
    व्योम visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        माला_दो(";\n");
    }
    व्योम visit(Ast_गाँठ_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        अगर (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            माला_दो("#0 ");
        } अन्यथा अगर (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            माला_दो("final ");
        } अन्यथा अगर (nodep->userType() == VAssertType::CONCURRENT) {
            माला_दो("property ");
        }
        iterateConstNull(nodep->sentreep());
        माला_दो("(");
        अगर (AstSampled* स्थिर sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_अग्र_Const_Null(sampledp->exprp());
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->propp());
        }
        अगर (!VN_IS(nodep, Restrict)) {
            माला_दो(") begin\n");
            iterate_And_अग्र_Const_Null(nodep->passsp());
            माला_दो("end\n");
        } अन्यथा {
            माला_दो(");\n");
        }

        अगर (स्थिर AstAssert* स्थिर assertp = VN_CAST(nodep, Assert)) {
            माला_दो("else begin\n");
            iterate_And_अग्र_Const_Null(assertp->failsp());
            माला_दो("end\n");
        } अन्यथा अगर (स्थिर AstAssertIntrinsic* स्थिर assertp = VN_CAST(nodep, AssertIntrinsic)) {
            माला_दो("else begin\n");
            iterate_And_अग्र_Const_Null(assertp->failsp());
            माला_दो("end\n");
        }
    }
    व्योम visit(AstAssocArrayDType* nodep) override {
        अगर (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = false;
            माला_दो("[");
            iterateConst(nodep->कुंजी_DTypep());
            माला_दो("]");
            m_arrayPost = true;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    व्योम visit(AstBasicDType* nodep) override {
        अगर (m_arrayPost) वापस;
        putfs(nodep, nodep->prettyName());
        अगर (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        अगर (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) वापस;
        emitPacked();
        अगर (nodep->rangep()) {
            माला_दो(" ");
            iterate_And_अग्र_Const_Null(nodep->rangep());
            माला_दो(" ");
        } अन्यथा अगर (nodep->isRanged()) {
            माला_दो(" [");
            माला_दो(cvtToStr(nodep->बाँए()));
            माला_दो(":");
            माला_दो(cvtToStr(nodep->दाँए()));
            माला_दो("] ");
        }
    }
    व्योम visit(AstConstDType* nodep) override {
        अगर (m_arrayPost) वापस;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    व्योम visit(AstDynArrayDType* nodep) override {
        अगर (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            माला_दो("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    व्योम visit(AstEnumDType* nodep) override {
        अगर (m_arrayPost) वापस;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        माला_दो("{\n");
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        माला_दो("}");
    }
    व्योम visit(AstEnumItemRef* nodep) override {
        अगर (Ast_गाँठ_Module* स्थिर classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            माला_दो("::");
        }
        putfs(nodep, nodep->name());
    }
    व्योम visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        माला_दो(" = ");
        iterateConstNull(nodep->valuep());
        अगर (nodep->nextp()) माला_दो(",");
        माला_दो("\n");
    }
    व्योम visit(Ast_गाँठ_Array_DType* nodep) override {
        अगर (!m_arrayPost) {
            अगर (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.धकेलो_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            अगर (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = false;
                iterate_And_अग्र_Const_Null(nodep->rangep());
                m_arrayPost = true;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    व्योम visit(AstIfaceRefDType* nodep) override {
        अगर (m_arrayPost) {
            माला_दो(" (");
            अगर (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } अन्यथा {
                माला_दो("????");
            }
            माला_दो(")");
            वापस;
        }
        माला_दो(nodep->ifaceName());
    }
    व्योम visit(AstRefDType* nodep) override {
        अगर (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } अन्यथा {
            माला_दो("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    व्योम visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    व्योम visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    व्योम visit(AstModport* nodep) override {
        माला_दो(nodep->verilogKwd());
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        माला_दो(" (\n");
        अगर (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } अन्यथा {
            माला_दो("????");
        }
        माला_दो(");\n");
    }
    व्योम visit(AstModportVarRef* nodep) override {
        माला_दो(nodep->direction().verilogKwd());
        माला_दो(" ");
        अगर (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = true;
            iterateConst(nodep->varp());
        } अन्यथा {
            माला_दो(nodep->prettyName());
        }
        अगर (nodep->nextp()) माला_दो(", ");
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        अगर (m_arrayPost) वापस;
        माला_दो(nodep->verilogKwd() + " ");
        अगर (nodep->packed()) माला_दो("packed ");
        {
            माला_दो("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            क्रम (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            माला_दो("}");
        }
        emitPacked();
    }
    व्योम visit(AstMemberDType* nodep) override {
        अगर (m_arrayPost) वापस;
        iterateConst(nodep->subDTypep());
        माला_दो(" ");
        माला_दो(nodep->name());
        माला_दो(";\n");
    }
    व्योम visit(AstQueueDType* nodep) override {
        अगर (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = false;
            माला_दो("[$");
            अगर (nodep->boundp()) {
                माला_दो(":");
                iterateConst(nodep->boundp());
            }
            माला_दो("]");
            m_arrayPost = true;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        अगर (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            माला_दो(".");
            माला_दो(nodep->prettyName());
        } अन्यथा {
            putfs(nodep, nodep->prettyName());
        }
        अगर (!VN_IS(nodep->taskp(), Property)) {
            माला_दो("(");
            iterate_And_अग्र_Const_Null(nodep->argsp());
            माला_दो(")");
            iterateConstNull(nodep->withp());
        }
    }
    व्योम visit(AstCCall* nodep) override {
        माला_दो(nodep->funcp()->name());
        माला_दो("(");
        iterate_And_अग्र_Const_Null(nodep->argsp());
        माला_दो(")");
    }
    व्योम visit(AstArg* nodep) override { iterate_And_अग्र_Const_Null(nodep->exprp()); }
    व्योम visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        माला_दो(") ");
    }
    व्योम visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    व्योम visit(AstPrintTimeScale* nodep) override {
        माला_दो(nodep->verilogKwd());
        माला_दो(";\n");
    }
    व्योम visit(AstPropSpec* nodep) override {
        अगर (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            क्रम (Ast_गाँठ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                अगर (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            माला_दो(")");
        }
        अगर (nodep->disablep()) {
            माला_दो(" disable iff ");
            iterateConst(nodep->disablep());
        }

        माला_दो(" ");
        अगर (nodep->propStrength() != VPropStrength::DEFAULT) {
            माला_दो(nodep->propStrength().ascii());
            माला_दो("(");
        }
        iterateConstNull(nodep->propp());
        अगर (nodep->propStrength() != VPropStrength::DEFAULT) माला_दो(")");
        माला_दो("\n");
    }
    व्योम visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    व्योम visit(AstAbortOn* nodep) override {
        माला_दो(nodep->kind().ascii());
        माला_दो(" (");
        iterateConst(nodep->condp());
        माला_दो(") ");
        iterateConst(nodep->propp());
    }
    व्योम visit(AstSClocked* nodep) override {
        माला_दो("@(");
        iterateConst(nodep->sensesp());
        माला_दो(") ");
        iterateConst(nodep->exprp());
        माला_दो("\n");
    }
    व्योम visit(AstPropAlways* nodep) override {
        माला_दो(nodep->isStrong() ? "s_always" : "always");
        अगर (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            माला_दो(" [");
            iterateConst(nodep->loBoundp());
            माला_दो(":");
            iterateConst(nodep->hiBoundp());
            माला_दो("]");
        }
        माला_दो(" ");
        iterateConst(nodep->propp());
    }
    व्योम visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = true;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    व्योम visit(AstVarRef* nodep) override {
        अगर (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } अन्यथा {
            अगर (nodep->varp()) {
                अगर (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } अन्यथा {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    माला_दो(nodep->varp()->prettyName());
                }
            } अन्यथा {
                putfs(nodep, nodep->name());
            }
        }
    }
    व्योम visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        माला_दो(".");
        अगर (nodep->varp()) {
            माला_दो(nodep->varp()->prettyName());
        } अन्यथा {
            माला_दो(nodep->prettyName());
        }
    }
    व्योम visit(AstConst* nodep) override { putfs(nodep, nodep->अंक().ascii(m_prefixed, true)); }
    व्योम visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    व्योम visit(Ast_शीर्ष_Scope* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    व्योम visit(AstVar* nodep) override {
        अगर (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            माला_दो(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = false;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        m_arrayPost = true;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        माला_दो(m_suppressVarSemi ? "\n" : ";\n");
    }
    व्योम visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    व्योम visit(AstDelay* nodep) override {
        माला_दो("");  // this is for proper alignment
        अगर (nodep->isCycleDelay()) {
            माला_दो("##");
        } अन्यथा {
            माला_दो("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = false;
        अगर (Ast_गाँठ_व्यंजक* स्थिर fallDelayp = nodep->fallDelay()) {
            माला_दो("(");
            iterateConst(nodep->lhsp());
            माला_दो(", ");
            iterateConst(fallDelayp);
            माला_दो(")");
        } अन्यथा {
            iterateConst(nodep->lhsp());
        }
        अगर (!m_suppressSemi) {
            माला_दो(";\n");
        } अन्यथा {
            माला_दो(" ");
        }
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    व्योम visit(AstWait* nodep) override {
        माला_दो("wait(");
        iterateConst(nodep->condp());
        माला_दो(")");
        अगर (nodep->stmtsp()) {
            माला_दो(" ");
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
        } अन्यथा {
            माला_दो(";\n");
        }
    }
    व्योम visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        माला_दो("");  // this is for proper alignment
        माला_दो("#");
        iterateConst(methodp->pinsp());
    }
    व्योम visit(AstParseRef* nodep) override { माला_दो(nodep->prettyName()); }
    व्योम visit(AstVarScope*) override {}
    व्योम visit(AstTraceDecl*) override {}
    व्योम visit(AstTraceInc*) override {}
    // NOPs
    व्योम visit(AstPragma*) override {}
    व्योम visit(AstStmtPragma*) override {}
    व्योम visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    व्योम visit(Ast_गाँठ* nodep) override {
        माला_दो("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        अगर (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

खुला:
    explicit EmitVBaseVisitorConst(bool suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = शेष;
};

//######################################################################
// Emit to an output file

श्रेणी EmitVFileVisitor final : खुला EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    व्योम putsNoTracking(स्थिर string& str) override { m_of.putsNoTracking(str); }
    व्योम माला_दो(स्थिर string& str) override { m_of.माला_दो(str); }
    व्योम putbs(स्थिर string& str) override { m_of.putbs(str); }
    व्योम putfs(Ast_गाँठ*, स्थिर string& str) override { putbs(str); }
    व्योम putqs(Ast_गाँठ*, स्थिर string& str) override { putbs(str); }

खुला:
    EmitVFileVisitor(Ast_गाँठ* nodep, V3OutVFile& of, bool suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = शेष;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

श्रेणी EmitVStreamVisitor final : खुला EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    स्थिर bool m_tracking;  // Use line tracking
    // METHODS
    व्योम putsNoTracking(स्थिर string& str) override { m_os.putsNoTracking(str); }
    व्योम माला_दो(स्थिर string& str) override {
        m_tracking ? m_os.माला_दो(str) : m_os.putsNoTracking(str);
    }
    व्योम putbs(स्थिर string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    व्योम putfs(Ast_गाँठ*, स्थिर string& str) override { putbs(str); }
    व्योम putqs(Ast_गाँठ*, स्थिर string& str) override { putbs(str); }

खुला:
    EmitVStreamVisitor(स्थिर Ast_गाँठ* nodep, std::ब_प्रवाह& os, bool tracking, bool suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_गाँठ*>(nodep));
    }
    ~EmitVStreamVisitor() override = शेष;
};

//######################################################################
// EmitV class functions

व्योम V3EmitV::verilog_For_वृक्ष(स्थिर Ast_गाँठ* nodep, std::ब_प्रवाह& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ false, false}; }
}

व्योम V3EmitV::debug_Verilog_For_वृक्ष(स्थिर Ast_गाँठ* nodep, std::ब_प्रवाह& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ true, true}; }
}

std::string V3EmitV::debug_Verilog_For_वृक्ष(स्थिर Ast_गाँठ* nodep) {
    std::stringstream ss;
    debug_Verilog_For_वृक्ष(nodep, ss);
    वापस ss.str();
}

व्योम V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    क्रम (Ast_गाँठ_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), गाँठ_File)) {
        AstVFile* स्थिर vfilep = VN_CAST(filep, VFile);
        अगर (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.माला_दो("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, false};
        }
    }
}

व्योम V3EmitV::debugEmitV(स्थिर string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, true};
}
