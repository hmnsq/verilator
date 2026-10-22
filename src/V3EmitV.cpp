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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitV.h"

#अन्तर्भाव "V3EmitCBase.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

वर्ग EmitVBaseVisitorConst VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    // STATE - across all visitors
    ध्रुव तार्किक m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_वृक्षः* m_sentreep = शून्यसूचकः;  // Domain for printing one a ALWAYS under a ACTIVE
    तार्किक m_suppressSemi = असत्य;  // Non-statement, don't print ;
    तार्किक m_suppressVarSemi = असत्य;  // Suppress emitting semicolon for AstVars
    तार्किक m_suppressSampled = असत्य;  // Suppress emitting sampled in assertion properties
    तार्किक m_arrayPost = असत्य;  // Print array information that goes after identifier (vs after)
    तार्किक m_prefixed = सत्य;  // Whether constants need to be prefixed
    std::deque<Ast_ग्रन्थिः_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_खण्डम्*, आकार_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    आभास शून्य पङ्क्तिं_लिख(ध्रुव string& str) = 0;
    आभास शून्य putbs(ध्रुव string& str) = 0;
    आभास शून्य putfs(Ast_ग्रन्थिः* nodep, ध्रुव string& str) = 0;  // Fileline and node %% mark
    आभास शून्य putqs(Ast_ग्रन्थिः* nodep, ध्रुव string& str) = 0;  // Fileline quiet w/o %% mark
    आभास शून्य putsNoTracking(ध्रुव string& str) = 0;
    आभास शून्य putsQuoted(ध्रुव string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_बहिर्गमनम्_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    शून्य iterateAndCommaConstNull(Ast_ग्रन्थिः* nodep) {
        प्रति (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            यदि (nodep->nextp()) पङ्क्तिं_लिख(", ");
        }
    }
    शून्य emitPacked() {
        प्रति (Ast_ग्रन्थिः_Array_DType* packedp : m_packedps) {
            पङ्क्तिं_लिख(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    शून्य emitNodesWithText(Ast_ग्रन्थिः* nodesp, तार्किक tracking, ध्रुव std::string& separator) {
        प्रति (Ast_ग्रन्थिः* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstText* ध्रुव textp = VN_CAST(nodep, Text)) {
                यदि (tracking) {
                    पङ्क्तिं_लिख(textp->text());
                } अन्यथा {
                    putsNoTracking(textp->text());
                }
            } अन्यथा {
                iterateConst(nodep);
            }
            यदि (nodep->nextp()) पङ्क्तिं_लिख(separator);
        }
    }

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override { iterate_And_अग्र_Const_Null(nodep->modulesp()); }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::उपसर्गः_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    शून्य दर्शय(Ast_पत्तनम्* nodep) override {}
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        पङ्क्तिं_लिख(" ");
        पङ्क्तिं_लिख(nodep->prettyName());
        पङ्क्तिं_लिख(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        ध्रुव std::string name = nodep->name().रिक्तानि() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        पङ्क्तिं_लिख("end" + name + '\n');
    }
    शून्य दर्शय(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख(")\n");
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    शून्य दर्शय(AstGenCaseItem* nodep) override {
        यदि (nodep->condsp()) {
            iterate_And_अग्र_Const_Null(nodep->condsp());
        } अन्यथा {
            putbs("default");
        }
        iterate_And_अग्र_Const_Null(nodep->itemsp());
    }
    शून्य दर्शय(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = सत्य;
            iterate_And_अग्र_Const_Null(nodep->initsp());
            पङ्क्तिं_लिख(";");
            iterate_And_अग्र_Const_Null(nodep->condp());
            पङ्क्तिं_लिख(";");
            iterate_And_अग्र_Const_Null(nodep->incsp());
        }
        पङ्क्तिं_लिख(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstGenIf* nodep) override {
        putfs(nodep, "");
        पङ्क्तिं_लिख("/* generate */ if (");
        iterate_And_अग्र_Const_Null(nodep->condp());
        पङ्क्तिं_लिख(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->thensp());
        यदि (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_अग्र_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstBegin* nodep) override {
        यदि (nodep->name() == "") {
            putbs("begin\n");
        } अन्यथा {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        पङ्क्तिं_लिख("end\n");
    }
    शून्य दर्शय(AstFork* nodep) override {
        यदि (nodep->name() == "") {
            putbs("fork\n");
        } अन्यथा {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        पङ्क्तिं_लिख(nodep->joinType().verilogKwd());
        पङ्क्तिं_लिख("\n");
    }
    शून्य दर्शय(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    शून्य दर्शय(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    शून्य दर्शय(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (ध्रुव Ast_नियोजय_W* ध्रुव ap = VN_CAST(nodep->stmtsp(), नियोजय_W)) {
            यदि (!ap->nextp()) {
                putfs(nodep, "assign ");
                यदि (Ast_ग्रन्थिः* ध्रुव tcp = ap->timingControlp()) {
                    iterate_And_अग्र_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_अग्र_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_अग्र_Const_Null(ap->rhsp());
                यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
                प्रत्यागम;
            }
        }
        putfs(nodep, "always ");
        यदि (m_sentreep) {
            iterate_And_अग्र_Const_Null(m_sentreep);
        }  // In active
        अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (VN_IS(nodep, नियोजय_Force)) पङ्क्तिं_लिख("force ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        प्रति (Ast_ग्रन्थिः* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstRelease* nodep) override {
        पङ्क्तिं_लिख("release ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstBreak*) override {
        putbs("break");
        यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        प्रति (Ast_ग्रन्थिः* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            यदि (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstSenItem* nodep) override {
        putfs(nodep, "");
        यदि (nodep->edgeType() != VEdgeType::ET_परिवर्तितम्) पङ्क्तिं_लिख(nodep->edgeType().verilogKwd());
        यदि (nodep->sensp()) पङ्क्तिं_लिख(" ");
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCReset* /*nodep*/) override { पङ्क्तिं_लिख("/*CRESET*/"); }
    शून्य दर्शय(AstCase* nodep) override {
        putfs(nodep, "");
        यदि (nodep->प्राथमिकता_Pragma()) पङ्क्तिं_लिख("priority ");
        यदि (nodep->uniquePragma()) पङ्क्तिं_लिख("unique ");
        यदि (nodep->unique0Pragma()) पङ्क्तिं_लिख("unique0 ");
        पङ्क्तिं_लिख(nodep->verilogKwd());
        पङ्क्तिं_लिख(" (");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख(")\n");
        यदि (nodep->पूर्णम्_Pragma() || nodep->parallelPragma()) {
            पङ्क्तिं_लिख(" // synopsys");
            यदि (nodep->पूर्णम्_Pragma()) पङ्क्तिं_लिख(" full_case");
            यदि (nodep->parallelPragma()) पङ्क्तिं_लिख(" parallel_case");
        }
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    शून्य दर्शय(AstCaseItem* nodep) override {
        यदि (nodep->condsp()) {
            iterate_And_अग्र_Const_Null(nodep->condsp());
        } अन्यथा {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstComment* nodep) override {
        पङ्क्तिं_लिख("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstContinue*) override {
        putbs("continue");
        यदि (!m_suppressSemi) पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Decl*) override {}  // N/A
    शून्य दर्शय(AstCoverInc*) override {}  // N/A
    शून्य दर्शय(AstCoverToggle*) override {}  // N/A

    शून्य दर्शय(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        यदि (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    शून्य दर्शय(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        यदि (nodep->binsp() || nodep->optionsp()) {
            पङ्क्तिं_लिख(" {\n");
            iterate_And_अग्र_Const_Null(nodep->optionsp());
            iterate_And_अग्र_Const_Null(nodep->binsp());
            पङ्क्तिं_लिख("}");
        }
        पङ्क्तिं_लिख(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    शून्य emitCoverageOption(Ast_ग्रन्थिः* nodep, तार्किक typeOption, VCoverOptionType optType,
                            Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(Ast_Cg_Option_नियोजय* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    शून्य दर्शय(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    शून्य दर्शय(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        पङ्क्तिं_लिख(nodep->name());
        यदि (nodep->isArray()) {
            पङ्क्तिं_लिख("[");
            यदि (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            पङ्क्तिं_लिख("]");
        }
        यदि (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            पङ्क्तिं_लिख(" = default");
        } अन्यथा यदि (nodep->transp()) {
            पङ्क्तिं_लिख(" = ");
            प्रति (Ast_ग्रन्थिः* setp = nodep->transp(); setp; setp = setp->nextp()) {
                यदि (setp != nodep->transp()) पङ्क्तिं_लिख(", ");
                iterateConst(setp);
            }
        } अन्यथा यदि (VN_IS(nodep->rangesp(), CoverWith)) {
            पङ्क्तिं_लिख(" = ");
            iterateConst(nodep->rangesp());
        } अन्यथा यदि (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            पङ्क्तिं_लिख(" = {");
            प्रति (Ast_ग्रन्थिः* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                यदि (rangep != nodep->rangesp()) पङ्क्तिं_लिख(", ");
                iterateConst(rangep);
            }
            पङ्क्तिं_लिख("}");
        }
        यदि (nodep->iffp()) {
            पङ्क्तिं_लिख(" iff (");
            iterateConst(nodep->iffp());
            पङ्क्तिं_लिख(")");
        }
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstCoverWith* nodep) override {
        ध्रुव तार्किक rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        यदि (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        यदि (rangeList) पङ्क्तिं_लिख("}");
        पङ्क्तिं_लिख(" with (");
        iterateConst(nodep->filterp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        यदि (!nodep->name().रिक्तानि()) पङ्क्तिं_लिख("." + nodep->name());
        पङ्क्तिं_लिख(")");
        यदि (nodep->rangesp()) {
            पङ्क्तिं_लिख(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            पङ्क्तिं_लिख("}");
        }
    }
    शून्य दर्शय(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        यदि (nodep->iffp()) {
            पङ्क्तिं_लिख(" iff (");
            iterateConst(nodep->iffp());
            पङ्क्तिं_लिख(")");
        }
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    शून्य दर्शय(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        यदि (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        पङ्क्तिं_लिख(")");
        यदि (nodep->countp()) {
            पङ्क्तिं_लिख(" matches ");
            iterateConst(nodep->countp());
            पङ्क्तिं_लिख(")");
        }
    }
    शून्य दर्शय(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    शून्य दर्शय(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        प्रति (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (itemp != nodep->itemsp()) पङ्क्तिं_लिख(", ");
            iterateConst(itemp);
        }
        यदि (nodep->iffp()) {
            पङ्क्तिं_लिख(" iff (");
            iterateConst(nodep->iffp());
            पङ्क्तिं_लिख(")");
        }
        यदि (nodep->binsp() || nodep->optionsp()) {
            पङ्क्तिं_लिख(" {\n");
            iterate_And_अग्र_Const_Null(nodep->optionsp());
            iterate_And_अग्र_Const_Null(nodep->binsp());
            पङ्क्तिं_लिख("}\n");
        } अन्यथा {
            पङ्क्तिं_लिख(";\n");
        }
    }
    शून्य दर्शय(Ast_Cover_Trans_समुच्चयः* nodep) override {
        पङ्क्तिं_लिख("(");
        प्रति (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (itemp != nodep->itemsp()) पङ्क्तिं_लिख(" => ");
            iterateConst(itemp);
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    शून्य दर्शय(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        यदि (AstConst* ध्रुव lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->अङ्कः().गन्तव्यम्_String());
        } अन्यथा {
            पङ्क्तिं_लिख("string'(");
            iterate_And_अग्र_Const_Null(nodep->lhsp());
            पङ्क्तिं_लिख(")");
        }
    }
    शून्य दर्शय(Ast_Test_Plus_तर्काः* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(Ast_मानम्_Plus_तर्काः* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय_ग्रन्थिः_Display(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* fileOrStrgp, ध्रुव string& text,
                          Ast_ग्रन्थिः* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        यदि (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        प्रति (Ast_ग्रन्थिः* expp = exprsp; expp; expp = expp->nextp()) {
            पङ्क्तिं_लिख(", ");
            iterateConstNull(expp);
        }
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    शून्य दर्शय(AstDisplay* nodep) override {
        दर्शय_ग्रन्थिः_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य दर्शय(AstElabDisplay* nodep) override {
        दर्शय_ग्रन्थिः_Display(nodep, शून्यसूचकः, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य दर्शय(AstFScanF* nodep) override {
        दर्शय_ग्रन्थिः_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        दर्शय_ग्रन्थिः_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    शून्य दर्शय(AstSFormat* nodep) override {
        दर्शय_ग्रन्थिः_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य दर्शय(Ast_गन्तव्यम्_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    शून्य दर्शय(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    शून्य दर्शय(AstSFormatF* nodep) override {
        दर्शय_ग्रन्थिः_Display(nodep, शून्यसूचकः, nodep->text(), nodep->exprsp());
    }
    शून्य दर्शय(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_अग्र_Const_Null(nodep->modep());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->filenamep());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        यदि (nodep->filep()) iterate_And_अग्र_Const_Null(nodep->filep());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        यदि (nodep->filep()) iterate_And_अग्र_Const_Null(nodep->filep());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        // Allocate label number
        ध्रुव आकार_t n = m_labelNumbers.आकार();
        ध्रुव तार्किक new_प्रविष्टिः = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(new_प्रविष्टिः, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::गन्तव्यम्_string(n) + "\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        पङ्क्तिं_लिख("end\n");
    }
    शून्य दर्शय(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ध्रुव स्वचालित it = m_labelNumbers.find(nodep->blockp());
        ध्रुव std::string label
            = it != m_labelNumbers.अन्तः_2() ? "label" + std::गन्तव्यम्_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        यदि (nodep->filenamep()) iterate_And_अग्र_Const_Null(nodep->filenamep());
        putbs(", ");
        यदि (nodep->memp()) iterate_And_अग्र_Const_Null(nodep->memp());
        यदि (nodep->lsbp()) {
            putbs(", ");
            iterate_And_अग्र_Const_Null(nodep->lsbp());
        }
        यदि (nodep->msbp()) {
            putbs(", ");
            iterate_And_अग्र_Const_Null(nodep->msbp());
        }
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->exprsp());
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_अग्र_Const_Null(nodep->countp());
        पङ्क्तिं_लिख(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    शून्य दर्शय(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        यदि (AstLoopTest* ध्रुव testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            पङ्क्तिं_लिख(") begin\n");
            iterate_And_अग्र_Const_Null(testp->nextp());
            पङ्क्तिं_लिख("end\n");
            प्रत्यागम;
        }
        // Special case when the AstLoopTest is last for output readability
        यदि (Ast_ग्रन्थिः* lastp = nodep->stmtsp()) {
            यावत् (Ast_ग्रन्थिः* ध्रुव nextp = lastp->nextp()) lastp = nextp;
            यदि (AstLoopTest* ध्रुव testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                प्रति (Ast_ग्रन्थिः* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                पङ्क्तिं_लिख("end while (");
                iterateConst(testp->condp());
                पङ्क्तिं_लिख(")\n");
                प्रत्यागम;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        iterate_And_अग्र_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_अग्र_Const_Null(nodep->condp());
        पङ्क्तिं_लिख(")) break;\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        putfs(nodep, "");
        यदि (ध्रुव AstIf* ध्रुव ifp = VN_CAST(nodep, If)) {
            यदि (ifp->प्राथमिकता_Pragma()) पङ्क्तिं_लिख("priority ");
            यदि (ifp->uniquePragma()) पङ्क्तिं_लिख("unique ");
            यदि (ifp->unique0Pragma()) पङ्क्तिं_लिख("unique0 ");
        }
        पङ्क्तिं_लिख("if (");
        iterate_And_अग्र_Const_Null(nodep->condp());
        पङ्क्तिं_लिख(") begin\n");
        iterate_And_अग्र_Const_Null(nodep->thensp());
        यदि (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_अग्र_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    शून्य दर्शय(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        यदि (nodep->ticksp() || nodep->sentreep()) {
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->ticksp());
            यदि (nodep->sentreep()) {
                पङ्क्तिं_लिख(", ");
                iterate_And_अग्र_Const_Null(nodep->sentreep());
            }
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstSampled* nodep) override {
        यदि (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_अग्र_Const_Null(nodep->exprp());
            पङ्क्तिं_लिख(")");
        }
    }
    शून्य दर्शय(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        यदि (nodep->sentreep()) {
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        यदि (nodep->sentreep()) {
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        यदि (nodep->sentreep()) {
            पङ्क्तिं_लिख(", ");
            iterate_And_अग्र_Const_Null(nodep->sentreep());
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_अग्र_Const_Null(nodep->exprp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    शून्य दर्शय(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        iterateConst(nodep->exprp());
        पङ्क्तिं_लिख(";\n");
    }

    // Nodes involing AstText
    शून्य दर्शय(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    शून्य दर्शय(Ast_Text_खण्डम्* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().रिक्तानि();
        पङ्क्तिं_लिख(nodep->उपसर्गः());
        emitNodesWithText(nodep->nodesp(), सत्य, nodep->separator());
        पङ्क्तिं_लिख(nodep->suffix());
    }
    शून्य दर्शय(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), सत्य, "");
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), सत्य, "");
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), असत्य, "");
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), असत्य, "");
        पङ्क्तिं_लिख(")");
    }

    शून्य दर्शय(AstScopeName* nodep) override {}
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_अग्र_Const_Null(nodep->resultp());
        पङ्क्तिं_लिख(");\n");
    }

    शून्य दर्शय(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        पङ्क्तिं_लिख("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        पङ्क्तिं_लिख(")");
        iterateConstNull(nodep->withp());
    }
    शून्य दर्शय(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        पङ्क्तिं_लिख("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        पङ्क्तिं_लिख(")");
    }

    // Operators
    आभास शून्य emitVerilogFormat(Ast_ग्रन्थिः* nodep, ध्रुव string& format, Ast_ग्रन्थिः* lhsp = शून्यसूचकः,
                                   Ast_ग्रन्थिः* ध्रुव rhsp = शून्यसूचकः, Ast_ग्रन्थिः* thsp = शून्यसूचकः,
                                   Ast_ग्रन्थिः* fhsp = शून्यसूचकः) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        तार्किक अन्तर्_Pct = असत्य;
        putbs("");
        प्रति (ध्रुव वर्ण c : format) {
            यदि (!अन्तर्_Pct && c == '%') {
                अन्तर्_Pct = सत्य;
            } अन्यथा यदि (!अन्तर्_Pct) {  // Normal text
                string s;
                s += c;
                पङ्क्तिं_लिख(s);
            } अन्यथा {  // Format character
                अन्तर्_Pct = असत्य;
                विकल्प (c) {
                पक्ष '%': पङ्क्तिं_लिख("%"); भङ्ग;
                पक्ष 'f': putfs(nodep, ""); भङ्ग;
                पक्ष 'k': putbs(""); भङ्ग;
                पक्ष 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(lhsp);
                    भङ्ग;
                }
                पक्ष 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(rhsp);
                    भङ्ग;
                }
                पक्ष 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(thsp);
                    भङ्ग;
                }
                पक्ष 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(fhsp);
                    भङ्ग;
                }
                पक्ष 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_अग्र_Const_Null(nodep->dtypep());
                    भङ्ग;
                }
                स्वभाव: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); भङ्ग;
                }
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    शून्य दर्शय(Ast_ग्रन्थिः_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        पङ्क्तिं_लिख(".");
        पङ्क्तिं_लिख(nodep->prettyName());
    }
    शून्य दर्शय(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        पङ्क्तिं_लिख(".");
        पङ्क्तिं_लिख(nodep->prettyName());
    }
    शून्य दर्शय(Ast_Attr_सम्बन्धि* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_अग्र_Const_Null(nodep->fromp());
        यदि (nodep->dimp()) {
            putbs(", ");
            iterate_And_अग्र_Const_Null(nodep->dimp());
        }
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        प्रति (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            यदि (itemp != nodep->itemsp()) putbs(", ");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(itemp->सूचकांक()));
            पङ्क्तिं_लिख(":");
            iterateConst(itemp->valuep());
        }
        पङ्क्तिं_लिख("}");
    }
    शून्य दर्शय(AstCond* nodep) override {
        putbs("(");
        iterate_And_अग्र_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_अग्र_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_अग्र_Const_Null(nodep->elsep());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstInsideRange* nodep) override {
        पङ्क्तिं_लिख("[");
        iterate_And_अग्र_Const_Null(nodep->lhsp());
        पङ्क्तिं_लिख(":");
        iterate_And_अग्र_Const_Null(nodep->rhsp());
        पङ्क्तिं_लिख("]");
    }
    शून्य दर्शय(AstRange* nodep) override {
        पङ्क्तिं_लिख("[");
        यदि (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->वामः_Const()));
            पङ्क्तिं_लिख(":");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->दक्षिणः_Const()));
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->leftp());
            पङ्क्तिं_लिख(":");
            iterate_And_अग्र_Const_Null(nodep->rightp());
        }
        पङ्क्तिं_लिख("]");
    }
    शून्य दर्शय(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    शून्य दर्शय(AstSel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        पूर्णाङ्क विस्थापनम् = 0;
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->fromp()->dtypep();
        यदि (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ध्रुव basicDtypep = VN_AS(dtypep, BasicDType);
            विस्थापनम् = basicDtypep->lo();
        }
        पङ्क्तिं_लिख("[");
        यदि (VN_IS(nodep->lsbp(), Const)) {
            यदि (nodep->विस्तारः_Const() == 1) {
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(VN_AS(nodep->lsbp(), Const)->गन्तव्यम्_SInt() + विस्थापनम्));
            } अन्यथा {
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(VN_AS(nodep->lsbp(), Const)->गन्तव्यम्_SInt() + nodep->विस्तारः_Const() + विस्थापनम्
                              - 1));
                पङ्क्तिं_लिख(":");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(VN_AS(nodep->lsbp(), Const)->गन्तव्यम्_SInt() + विस्थापनम्));
            }
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->lsbp());
            यदि (विस्थापनम् != 0) {
                पङ्क्तिं_लिख(" + ");
                पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(विस्थापनम्));
            }
            putfs(nodep, "+:");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->विस्तारः_Const()));
        }
        पङ्क्तिं_लिख("]");
    }
    शून्य दर्शय(Ast_खण्डिका_Sel* nodep) override {
        iterate_And_अग्र_Const_Null(nodep->fromp());
        पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->declRange()));
    }
    शून्य दर्शय(AstThisRef* /*nodep*/) override { पङ्क्तिं_लिख("this"); }
    शून्य दर्शय(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        पङ्क्तिं_लिख(" ");
        पङ्क्तिं_लिख(nodep->prettyName());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        यदि (nodep->userType() == VAssertType::OBSERVED_DEFERRED_तात्कालिकम्) {
            पङ्क्तिं_लिख("#0 ");
        } अन्यथा यदि (nodep->userType() == VAssertType::FINAL_DEFERRED_तात्कालिकम्) {
            पङ्क्तिं_लिख("final ");
        } अन्यथा यदि (nodep->userType() == VAssertType::CONCURRENT) {
            पङ्क्तिं_लिख("property ");
        }
        iterateConstNull(nodep->sentreep());
        पङ्क्तिं_लिख("(");
        यदि (AstSampled* ध्रुव sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_अग्र_Const_Null(sampledp->exprp());
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->propp());
        }
        यदि (!VN_IS(nodep, Restrict)) {
            पङ्क्तिं_लिख(") begin\n");
            iterate_And_अग्र_Const_Null(nodep->passsp());
            पङ्क्तिं_लिख("end\n");
        } अन्यथा {
            पङ्क्तिं_लिख(");\n");
        }

        यदि (ध्रुव AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            पङ्क्तिं_लिख("else begin\n");
            iterate_And_अग्र_Const_Null(assertp->failsp());
            पङ्क्तिं_लिख("end\n");
        } अन्यथा यदि (ध्रुव AstAssertIntrinsic* ध्रुव assertp = VN_CAST(nodep, AssertIntrinsic)) {
            पङ्क्तिं_लिख("else begin\n");
            iterate_And_अग्र_Const_Null(assertp->failsp());
            पङ्क्तिं_लिख("end\n");
        }
    }
    शून्य दर्शय(AstAssocArrayDType* nodep) override {
        यदि (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = असत्य;
            पङ्क्तिं_लिख("[");
            iterateConst(nodep->कुञ्चिका_DTypep());
            पङ्क्तिं_लिख("]");
            m_arrayPost = सत्य;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य दर्शय(AstBasicDType* nodep) override {
        यदि (m_arrayPost) प्रत्यागम;
        putfs(nodep, nodep->prettyName());
        यदि (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        यदि (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) प्रत्यागम;
        emitPacked();
        यदि (nodep->rangep()) {
            पङ्क्तिं_लिख(" ");
            iterate_And_अग्र_Const_Null(nodep->rangep());
            पङ्क्तिं_लिख(" ");
        } अन्यथा यदि (nodep->isRanged()) {
            पङ्क्तिं_लिख(" [");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->वामतः()));
            पङ्क्तिं_लिख(":");
            पङ्क्तिं_लिख(cvt_गन्तव्यम्_Str(nodep->दक्षिणतः()));
            पङ्क्तिं_लिख("] ");
        }
    }
    शून्य दर्शय(AstConstDType* nodep) override {
        यदि (m_arrayPost) प्रत्यागम;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    शून्य दर्शय(AstDynArrayDType* nodep) override {
        यदि (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            पङ्क्तिं_लिख("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य दर्शय(AstEnumDType* nodep) override {
        यदि (m_arrayPost) प्रत्यागम;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        पङ्क्तिं_लिख("{\n");
        iterate_And_अग्र_Const_Null(nodep->itemsp());
        पङ्क्तिं_लिख("}");
    }
    शून्य दर्शय(AstEnumItemRef* nodep) override {
        यदि (Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            पङ्क्तिं_लिख("::");
        }
        putfs(nodep, nodep->name());
    }
    शून्य दर्शय(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        पङ्क्तिं_लिख(" = ");
        iterateConstNull(nodep->valuep());
        यदि (nodep->nextp()) पङ्क्तिं_लिख(",");
        पङ्क्तिं_लिख("\n");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Array_DType* nodep) override {
        यदि (!m_arrayPost) {
            यदि (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.प्रक्षिप_पृष्ठभागः(nodep);
            }
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            यदि (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = असत्य;
                iterate_And_अग्र_Const_Null(nodep->rangep());
                m_arrayPost = सत्य;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य दर्शय(AstIfaceRefDType* nodep) override {
        यदि (m_arrayPost) {
            पङ्क्तिं_लिख(" (");
            यदि (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } अन्यथा {
                पङ्क्तिं_लिख("????");
            }
            पङ्क्तिं_लिख(")");
            प्रत्यागम;
        }
        पङ्क्तिं_लिख(nodep->ifaceName());
    }
    शून्य दर्शय(AstRefDType* nodep) override {
        यदि (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } अन्यथा {
            पङ्क्तिं_लिख("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    शून्य दर्शय(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::उपसर्गः_Name_Protect(nodep->classp()));
    }
    शून्य दर्शय(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    शून्य दर्शय(AstModport* nodep) override {
        पङ्क्तिं_लिख(nodep->verilogKwd());
        पङ्क्तिं_लिख(" ");
        पङ्क्तिं_लिख(nodep->prettyName());
        पङ्क्तिं_लिख(" (\n");
        यदि (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } अन्यथा {
            पङ्क्तिं_लिख("????");
        }
        पङ्क्तिं_लिख(");\n");
    }
    शून्य दर्शय(AstModportVarRef* nodep) override {
        पङ्क्तिं_लिख(nodep->direction().verilogKwd());
        पङ्क्तिं_लिख(" ");
        यदि (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = सत्य;
            iterateConst(nodep->varp());
        } अन्यथा {
            पङ्क्तिं_लिख(nodep->prettyName());
        }
        यदि (nodep->nextp()) पङ्क्तिं_लिख(", ");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        यदि (m_arrayPost) प्रत्यागम;
        पङ्क्तिं_लिख(nodep->verilogKwd() + " ");
        यदि (nodep->packed()) पङ्क्तिं_लिख("packed ");
        {
            पङ्क्तिं_लिख("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            प्रति (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            पङ्क्तिं_लिख("}");
        }
        emitPacked();
    }
    शून्य दर्शय(AstMemberDType* nodep) override {
        यदि (m_arrayPost) प्रत्यागम;
        iterateConst(nodep->subDTypep());
        पङ्क्तिं_लिख(" ");
        पङ्क्तिं_लिख(nodep->name());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstQueueDType* nodep) override {
        यदि (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = असत्य;
            पङ्क्तिं_लिख("[$");
            यदि (nodep->boundp()) {
                पङ्क्तिं_लिख(":");
                iterateConst(nodep->boundp());
            }
            पङ्क्तिं_लिख("]");
            m_arrayPost = सत्य;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            पङ्क्तिं_लिख(".");
            पङ्क्तिं_लिख(nodep->prettyName());
        } अन्यथा {
            putfs(nodep, nodep->prettyName());
        }
        यदि (!VN_IS(nodep->taskp(), Property)) {
            पङ्क्तिं_लिख("(");
            iterate_And_अग्र_Const_Null(nodep->argsp());
            पङ्क्तिं_लिख(")");
            iterateConstNull(nodep->withp());
        }
    }
    शून्य दर्शय(AstCCall* nodep) override {
        पङ्क्तिं_लिख(nodep->funcp()->name());
        पङ्क्तिं_लिख("(");
        iterate_And_अग्र_Const_Null(nodep->argsp());
        पङ्क्तिं_लिख(")");
    }
    शून्य दर्शय(AstArg* nodep) override { iterate_And_अग्र_Const_Null(nodep->exprp()); }
    शून्य दर्शय(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        पङ्क्तिं_लिख(") ");
    }
    शून्य दर्शय(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    शून्य दर्शय(AstPrintTimeScale* nodep) override {
        पङ्क्तिं_लिख(nodep->verilogKwd());
        पङ्क्तिं_लिख(";\n");
    }
    शून्य दर्शय(AstPropSpec* nodep) override {
        यदि (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            प्रति (Ast_ग्रन्थिः* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                यदि (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            पङ्क्तिं_लिख(")");
        }
        यदि (nodep->disablep()) {
            पङ्क्तिं_लिख(" disable iff ");
            iterateConst(nodep->disablep());
        }

        पङ्क्तिं_लिख(" ");
        यदि (nodep->propStrength() != VPropStrength::DEFAULT) {
            पङ्क्तिं_लिख(nodep->propStrength().ascii());
            पङ्क्तिं_लिख("(");
        }
        iterateConstNull(nodep->propp());
        यदि (nodep->propStrength() != VPropStrength::DEFAULT) पङ्क्तिं_लिख(")");
        पङ्क्तिं_लिख("\n");
    }
    शून्य दर्शय(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    शून्य दर्शय(AstAbortOn* nodep) override {
        पङ्क्तिं_लिख(nodep->विधा().ascii());
        पङ्क्तिं_लिख(" (");
        iterateConst(nodep->condp());
        पङ्क्तिं_लिख(") ");
        iterateConst(nodep->propp());
    }
    शून्य दर्शय(AstSClocked* nodep) override {
        पङ्क्तिं_लिख("@(");
        iterateConst(nodep->sensesp());
        पङ्क्तिं_लिख(") ");
        iterateConst(nodep->exprp());
        पङ्क्तिं_लिख("\n");
    }
    शून्य दर्शय(AstPropAlways* nodep) override {
        पङ्क्तिं_लिख(nodep->isStrong() ? "s_always" : "always");
        यदि (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            पङ्क्तिं_लिख(" [");
            iterateConst(nodep->loBoundp());
            पङ्क्तिं_लिख(":");
            iterateConst(nodep->hiBoundp());
            पङ्क्तिं_लिख("]");
        }
        पङ्क्तिं_लिख(" ");
        iterateConst(nodep->propp());
    }
    शून्य दर्शय(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = सत्य;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } अन्यथा {
            यदि (nodep->varp()) {
                यदि (nodep->selfPointer().is_रिक्तानि()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } अन्यथा {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    पङ्क्तिं_लिख(nodep->varp()->prettyName());
                }
            } अन्यथा {
                putfs(nodep, nodep->name());
            }
        }
    }
    शून्य दर्शय(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        पङ्क्तिं_लिख(".");
        यदि (nodep->varp()) {
            पङ्क्तिं_लिख(nodep->varp()->prettyName());
        } अन्यथा {
            पङ्क्तिं_लिख(nodep->prettyName());
        }
    }
    शून्य दर्शय(AstConst* nodep) override { putfs(nodep, nodep->अङ्कः().ascii(m_prefixed, सत्य)); }
    शून्य दर्शय(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    शून्य दर्शय(Ast_शिखरम्_Scope* nodep) override { iterateChildrenConst(nodep); }
    शून्य दर्शय(AstScope* nodep) override { iterateChildrenConst(nodep); }
    शून्य दर्शय(AstVar* nodep) override {
        यदि (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            पङ्क्तिं_लिख(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = असत्य;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        पङ्क्तिं_लिख(" ");
        पङ्क्तिं_लिख(nodep->prettyName());
        m_arrayPost = सत्य;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        पङ्क्तिं_लिख(m_suppressVarSemi ? "\n" : ";\n");
    }
    शून्य दर्शय(Ast_सक्रिय* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    शून्य दर्शय(AstDelay* nodep) override {
        पङ्क्तिं_लिख("");  // this is for proper alignment
        यदि (nodep->is_आवृत्तिः_Delay()) {
            पङ्क्तिं_लिख("##");
        } अन्यथा {
            पङ्क्तिं_लिख("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = असत्य;
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fallDelayp = nodep->fallDelay()) {
            पङ्क्तिं_लिख("(");
            iterateConst(nodep->lhsp());
            पङ्क्तिं_लिख(", ");
            iterateConst(fallDelayp);
            पङ्क्तिं_लिख(")");
        } अन्यथा {
            iterateConst(nodep->lhsp());
        }
        यदि (!m_suppressSemi) {
            पङ्क्तिं_लिख(";\n");
        } अन्यथा {
            पङ्क्तिं_लिख(" ");
        }
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }
    शून्य दर्शय(AstWait* nodep) override {
        पङ्क्तिं_लिख("wait(");
        iterateConst(nodep->condp());
        पङ्क्तिं_लिख(")");
        यदि (nodep->stmtsp()) {
            पङ्क्तिं_लिख(" ");
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
        } अन्यथा {
            पङ्क्तिं_लिख(";\n");
        }
    }
    शून्य दर्शय(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        पङ्क्तिं_लिख("");  // this is for proper alignment
        पङ्क्तिं_लिख("#");
        iterateConst(methodp->pinsp());
    }
    शून्य दर्शय(AstParseRef* nodep) override { पङ्क्तिं_लिख(nodep->prettyName()); }
    शून्य दर्शय(AstVarScope*) override {}
    शून्य दर्शय(AstTraceDecl*) override {}
    शून्य दर्शय(AstTraceInc*) override {}
    // NOPs
    शून्य दर्शय(AstPragma*) override {}
    शून्य दर्शय(AstStmtPragma*) override {}
    शून्य दर्शय(AstCell*) override {}  // Handled outside the Visit class
    // Default
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        पङ्क्तिं_लिख("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        यदि (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

सार्वजनिक:
    स्पष्ट EmitVBaseVisitorConst(तार्किक suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = स्वभाव;
};

//######################################################################
// Emit to an output file

वर्ग EmitVFileVisitor final : सार्वजनिक EmitVBaseVisitorConst {
    // STATE
    V3_बहिर्गमनम्_VFile& m_सम्बन्धि;  // The output file
    // METHODS
    शून्य putsNoTracking(ध्रुव string& str) override { m_सम्बन्धि.putsNoTracking(str); }
    शून्य पङ्क्तिं_लिख(ध्रुव string& str) override { m_सम्बन्धि.पङ्क्तिं_लिख(str); }
    शून्य putbs(ध्रुव string& str) override { m_सम्बन्धि.putbs(str); }
    शून्य putfs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }
    शून्य putqs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }

सार्वजनिक:
    EmitVFileVisitor(Ast_ग्रन्थिः* nodep, V3_बहिर्गमनम्_VFile& सम्बन्धि, तार्किक suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_सम्बन्धि{सम्बन्धि} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = स्वभाव;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

वर्ग EmitVStreamVisitor final : सार्वजनिक EmitVBaseVisitorConst {
    // STATE
    V3_बहिर्गमनम्_प्रवाहः m_os;  // The output stream formatter
    ध्रुव तार्किक m_tracking;  // Use line tracking
    // METHODS
    शून्य putsNoTracking(ध्रुव string& str) override { m_os.putsNoTracking(str); }
    शून्य पङ्क्तिं_लिख(ध्रुव string& str) override {
        m_tracking ? m_os.पङ्क्तिं_लिख(str) : m_os.putsNoTracking(str);
    }
    शून्य putbs(ध्रुव string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    शून्य putfs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }
    शून्य putqs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }

सार्वजनिक:
    EmitVStreamVisitor(ध्रुव Ast_ग्रन्थिः* nodep, std::ostream& os, तार्किक tracking, तार्किक suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_बहिर्गमनम्_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ग्रन्थिः*>(nodep));
    }
    ~EmitVStreamVisitor() override = स्वभाव;
};

//######################################################################
// EmitV class functions

शून्य V3EmitV::verilog_For_वृक्षः(ध्रुव Ast_ग्रन्थिः* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ असत्य, असत्य}; }
}

शून्य V3EmitV::debug_Verilog_For_वृक्षः(ध्रुव Ast_ग्रन्थिः* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ सत्य, सत्य}; }
}

std::string V3EmitV::debug_Verilog_For_वृक्षः(ध्रुव Ast_ग्रन्थिः* nodep) {
    std::stringstream ss;
    debug_Verilog_For_वृक्षः(nodep, ss);
    प्रत्यागम ss.str();
}

शून्य V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    प्रति (Ast_ग्रन्थिः_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ग्रन्थिः_File)) {
        AstVFile* ध्रुव vfilep = VN_CAST(filep, VFile);
        यदि (vfilep && vfilep->tblockp()) {
            V3_बहिर्गमनम्_VFile सम्बन्धि{vfilep->name()};
            सम्बन्धि.पङ्क्तिं_लिख("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), सम्बन्धि, असत्य};
        }
    }
}

शून्य V3EmitV::debugEmitV(ध्रुव string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_बहिर्गमनम्_VFile सम्बन्धि{filename};
    EmitVFileVisitor{v3Global.rootp(), सम्बन्धि, सत्य};
}
