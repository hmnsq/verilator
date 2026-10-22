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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3EmitV.h"

#అన్తర్భావ "V3EmitCBase.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

తరగతి EmitVBaseVisitorConst VL_NOT_FINAL : బహిరంగ VNVisitorConst {
    // STATE - across all visitors
    స్థిరరాశి తార్కిక m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_వృక్షం* m_sentreep = శూన్యసూచిక;  // Domain for printing one a ALWAYS under a ACTIVE
    తార్కిక m_suppressSemi = అబద్ధం;  // Non-statement, don't print ;
    తార్కిక m_suppressVarSemi = అబద్ధం;  // Suppress emitting semicolon for AstVars
    తార్కిక m_suppressSampled = అబద్ధం;  // Suppress emitting sampled in assertion properties
    తార్కిక m_arrayPost = అబద్ధం;  // Print array information that goes after identifier (vs after)
    తార్కిక m_prefixed = నిజం;  // Whether constants need to be prefixed
    std::deque<Ast_కణుపు_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_ఖండం*, కొలత_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    మాయా శూన్య puts(స్థిరరాశి string& str) = 0;
    మాయా శూన్య putbs(స్థిరరాశి string& str) = 0;
    మాయా శూన్య putfs(Ast_కణుపు* nodep, స్థిరరాశి string& str) = 0;  // Fileline and node %% mark
    మాయా శూన్య putqs(Ast_కణుపు* nodep, స్థిరరాశి string& str) = 0;  // Fileline quiet w/o %% mark
    మాయా శూన్య putsNoTracking(స్థిరరాశి string& str) = 0;
    మాయా శూన్య putsQuoted(స్థిరరాశి string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_బహిర్గతం_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    శూన్య iterateAndCommaConstNull(Ast_కణుపు* nodep) {
        కొరకు (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            ఒకవేళ (nodep->nextp()) puts(", ");
        }
    }
    శూన్య emitPacked() {
        కొరకు (Ast_కణుపు_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    శూన్య emit_Nodes_With_పాఠ్యం(Ast_కణుపు* nodesp, తార్కిక tracking, స్థిరరాశి std::string& separator) {
        కొరకు (Ast_కణుపు* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (స్థిరరాశి Ast_పాఠ్యం* స్థిరరాశి textp = VN_CAST(nodep, పాఠ్యం_2)) {
                ఒకవేళ (tracking) {
                    puts(textp->పాఠ్యం());
                } లేకపోతే {
                    putsNoTracking(textp->పాఠ్యం());
                }
            } లేకపోతే {
                iterateConst(nodep);
            }
            ఒకవేళ (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override { iterate_And_తదుపరి_Const_Null(nodep->modulesp()); }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::ఉపసర్గ_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    శూన్య సందర్శించు(Ast_పోర్టు* nodep) override {}
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        స్థిరరాశి std::string name = nodep->name().ఖాళీలు() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    శూన్య సందర్శించు(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_తదుపరి_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    శూన్య సందర్శించు(Ast_Gen_Case_అంశం* nodep) override {
        ఒకవేళ (nodep->condsp()) {
            iterate_And_తదుపరి_Const_Null(nodep->condsp());
        } లేకపోతే {
            putbs("default");
        }
        iterate_And_తదుపరి_Const_Null(nodep->itemsp());
    }
    శూన్య సందర్శించు(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = నిజం;
            iterate_And_తదుపరి_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_తదుపరి_Const_Null(nodep->condp());
            puts(";");
            iterate_And_తదుపరి_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        ఒకవేళ (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_తదుపరి_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        ఒకవేళ (nodep->name() == "") {
            putbs("begin\n");
        } లేకపోతే {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        ఒకవేళ (nodep->name() == "") {
            putbs("fork\n");
        } లేకపోతే {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    శూన్య సందర్శించు(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి ap = VN_CAST(nodep->stmtsp(), కేటాయించు_W)) {
            ఒకవేళ (!ap->nextp()) {
                putfs(nodep, "assign ");
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి tcp = ap->timingControlp()) {
                    iterate_And_తదుపరి_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_తదుపరి_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_తదుపరి_Const_Null(ap->rhsp());
                ఒకవేళ (!m_suppressSemi) puts(";\n");
                తిరిగి;
            }
        }
        putfs(nodep, "always ");
        ఒకవేళ (m_sentreep) {
            iterate_And_తదుపరి_Const_Null(m_sentreep);
        }  // In active
        లేకపోతే {
            iterate_And_తదుపరి_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (VN_IS(nodep, కేటాయించు_Force)) puts("force ");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_తదుపరి_Const_Null(nodep->rhsp());
        ఒకవేళ (!m_suppressSemi) puts(";\n");
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        కొరకు (Ast_కణుపు* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        ఒకవేళ (!m_suppressSemi) puts(";\n");
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_తదుపరి_Const_Null(nodep->rhsp());
        ఒకవేళ (!m_suppressSemi) puts(";\n");
    }
    శూన్య సందర్శించు(AstRelease* nodep) override {
        puts("release ");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        ఒకవేళ (!m_suppressSemi) puts(";\n");
    }
    శూన్య సందర్శించు(AstBreak*) override {
        putbs("break");
        ఒకవేళ (!m_suppressSemi) puts(";\n");
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        కొరకు (Ast_కణుపు* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            ఒకవేళ (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        putfs(nodep, "");
        ఒకవేళ (nodep->అంచు_Type() != VEdgeType::ET_మారింది) puts(nodep->అంచు_Type().verilogKwd());
        ఒకవేళ (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    శూన్య సందర్శించు(AstCase* nodep) override {
        putfs(nodep, "");
        ఒకవేళ (nodep->ప్రాధాన్యత_Pragma()) puts("priority ");
        ఒకవేళ (nodep->uniquePragma()) puts("unique ");
        ఒకవేళ (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts(")\n");
        ఒకవేళ (nodep->నిండిన_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            ఒకవేళ (nodep->నిండిన_Pragma()) puts(" full_case");
            ఒకవేళ (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_తదుపరి_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        ఒకవేళ (nodep->condsp()) {
            iterate_And_తదుపరి_Const_Null(nodep->condsp());
        } లేకపోతే {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstContinue*) override {
        putbs("continue");
        ఒకవేళ (!m_suppressSemi) puts(";\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Decl*) override {}  // N/A
    శూన్య సందర్శించు(AstCoverInc*) override {}  // N/A
    శూన్య సందర్శించు(AstCoverToggle*) override {}  // N/A

    శూన్య సందర్శించు(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        ఒకవేళ (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    శూన్య సందర్శించు(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        ఒకవేళ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_తదుపరి_Const_Null(nodep->optionsp());
            iterate_And_తదుపరి_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    శూన్య emitCoverageOption(Ast_కణుపు* nodep, తార్కిక typeOption, VCoverOptionType optType,
                            Ast_కణుపు_సమాసం* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    శూన్య సందర్శించు(Ast_Cg_Option_కేటాయించు* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    శూన్య సందర్శించు(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    శూన్య సందర్శించు(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        ఒకవేళ (nodep->isArray()) {
            puts("[");
            ఒకవేళ (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        ఒకవేళ (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } లేకపోతే ఒకవేళ (nodep->transp()) {
            puts(" = ");
            కొరకు (Ast_కణుపు* setp = nodep->transp(); setp; setp = setp->nextp()) {
                ఒకవేళ (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } లేకపోతే ఒకవేళ (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } లేకపోతే ఒకవేళ (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            కొరకు (Ast_కణుపు* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                ఒకవేళ (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        ఒకవేళ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    శూన్య సందర్శించు(AstCoverWith* nodep) override {
        స్థిరరాశి తార్కిక rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        ఒకవేళ (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        ఒకవేళ (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    శూన్య సందర్శించు(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        ఒకవేళ (!nodep->name().ఖాళీలు()) puts("." + nodep->name());
        puts(")");
        ఒకవేళ (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    శూన్య సందర్శించు(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        ఒకవేళ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    శూన్య సందర్శించు(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    శూన్య సందర్శించు(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    శూన్య సందర్శించు(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        ఒకవేళ (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        ఒకవేళ (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    శూన్య సందర్శించు(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    శూన్య సందర్శించు(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        కొరకు (Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        ఒకవేళ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        ఒకవేళ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_తదుపరి_Const_Null(nodep->optionsp());
            iterate_And_తదుపరి_Const_Null(nodep->binsp());
            puts("}\n");
        } లేకపోతే {
            puts(";\n");
        }
    }
    శూన్య సందర్శించు(Ast_Cover_Trans_సమితి* nodep) override {
        puts("(");
        కొరకు (Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    శూన్య సందర్శించు(Ast_Cover_Trans_అంశం* nodep) override { iterateChildrenConst(nodep); }

    శూన్య సందర్శించు(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        ఒకవేళ (AstConst* స్థిరరాశి lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->అంకె().గమ్యం_String());
        } లేకపోతే {
            puts("string'(");
            iterate_And_తదుపరి_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    శూన్య సందర్శించు(Ast_Test_Plus_వాదనలు* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    శూన్య సందర్శించు(Ast_విలువ_Plus_వాదనలు* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    శూన్య సందర్శించు_కణుపు_Display(Ast_కణుపు* nodep, Ast_కణుపు* fileOrStrgp, స్థిరరాశి string& పాఠ్యం,
                          Ast_కణుపు* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ఒకవేళ (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(పాఠ్యం);
        కొరకు (Ast_కణుపు* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    శూన్య సందర్శించు(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    శూన్య సందర్శించు(AstDisplay* nodep) override {
        సందర్శించు_కణుపు_Display(nodep, nodep->filep(), nodep->fmtp()->పాఠ్యం(), nodep->fmtp()->exprsp());
    }
    శూన్య సందర్శించు(AstElabDisplay* nodep) override {
        సందర్శించు_కణుపు_Display(nodep, శూన్యసూచిక, nodep->fmtp()->పాఠ్యం(), nodep->fmtp()->exprsp());
    }
    శూన్య సందర్శించు(AstFScanF* nodep) override {
        సందర్శించు_కణుపు_Display(nodep, nodep->filep(), nodep->పాఠ్యం(), nodep->exprsp());
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        సందర్శించు_కణుపు_Display(nodep, nodep->fromp(), nodep->పాఠ్యం(), nodep->exprsp());
    }
    శూన్య సందర్శించు(AstSFormat* nodep) override {
        సందర్శించు_కణుపు_Display(nodep, nodep->lhsp(), nodep->fmtp()->పాఠ్యం(), nodep->fmtp()->exprsp());
    }
    శూన్య సందర్శించు(Ast_గమ్యం_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    శూన్య సందర్శించు(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        సందర్శించు_కణుపు_Display(nodep, శూన్యసూచిక, nodep->పాఠ్యం(), nodep->exprsp());
    }
    శూన్య సందర్శించు(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_తదుపరి_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_తదుపరి_Const_Null(nodep->modep());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_తదుపరి_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ఒకవేళ (nodep->filep()) iterate_And_తదుపరి_Const_Null(nodep->filep());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ఒకవేళ (nodep->filep()) iterate_And_తదుపరి_Const_Null(nodep->filep());
        puts(");\n");
    }
    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        // Allocate label number
        స్థిరరాశి కొలత_t n = m_labelNumbers.కొలత();
        స్థిరరాశి తార్కిక new_నమోదు = m_labelNumbers.emplace(nodep, n).రెండవ;
        UASSERT_OBJ(new_నమోదు, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::గమ్యం_string(n) + "\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        స్థిరరాశి స్వయంచాలక it = m_labelNumbers.కనుగొను(nodep->blockp());
        స్థిరరాశి std::string label
            = it != m_labelNumbers.చివర() ? "label" + std::గమ్యం_string(it->రెండవ) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ఒకవేళ (nodep->filenamep()) iterate_And_తదుపరి_Const_Null(nodep->filenamep());
        putbs(", ");
        ఒకవేళ (nodep->memp()) iterate_And_తదుపరి_Const_Null(nodep->memp());
        ఒకవేళ (nodep->lsbp()) {
            putbs(", ");
            iterate_And_తదుపరి_Const_Null(nodep->lsbp());
        }
        ఒకవేళ (nodep->msbp()) {
            putbs(", ");
            iterate_And_తదుపరి_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    శూన్య సందర్శించు(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_తదుపరి_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    శూన్య సందర్శించు(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_తదుపరి_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_తదుపరి_Const_Null(testp->nextp());
            puts("end\n");
            తిరిగి;
        }
        // Special case when the AstLoopTest is last for output readability
        ఒకవేళ (Ast_కణుపు* lastp = nodep->stmtsp()) {
            అయితే (Ast_కణుపు* స్థిరరాశి nextp = lastp->nextp()) lastp = nextp;
            ఒకవేళ (AstLoopTest* స్థిరరాశి testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                కొరకు (Ast_కణుపు* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                తిరిగి;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        iterate_And_తదుపరి_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        putfs(nodep, "");
        ఒకవేళ (స్థిరరాశి AstIf* స్థిరరాశి ifp = VN_CAST(nodep, If)) {
            ఒకవేళ (ifp->ప్రాధాన్యత_Pragma()) puts("priority ");
            ఒకవేళ (ifp->uniquePragma()) puts("unique ");
            ఒకవేళ (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        ఒకవేళ (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_తదుపరి_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    శూన్య సందర్శించు(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        ఒకవేళ (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->ticksp());
            ఒకవేళ (nodep->sentreep()) {
                puts(", ");
                iterate_And_తదుపరి_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    శూన్య సందర్శించు(AstSampled* nodep) override {
        ఒకవేళ (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_తదుపరి_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    శూన్య సందర్శించు(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts(")");
    }
    శూన్య సందర్శించు(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        ఒకవేళ (nodep->sentreep()) {
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    శూన్య సందర్శించు(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        ఒకవేళ (nodep->sentreep()) {
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    శూన్య సందర్శించు(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts(")");
    }
    శూన్య సందర్శించు(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts(")");
    }
    శూన్య సందర్శించు(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        ఒకవేళ (nodep->sentreep()) {
            puts(", ");
            iterate_And_తదుపరి_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    శూన్య సందర్శించు(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
        puts(")");
    }
    శూన్య సందర్శించు(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    శూన్య సందర్శించు(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    శూన్య సందర్శించు(Ast_ముగింపు* nodep) override { putfs(nodep, "$finish;\n"); }
    శూన్య సందర్శించు(Ast_ముగింపు_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    శూన్య సందర్శించు(Ast_పాఠ్యం* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    శూన్య సందర్శించు(Ast_పాఠ్యం_ఖండం* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().ఖాళీలు();
        puts(nodep->ఉపసర్గ());
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), నిజం, nodep->separator());
        puts(nodep->suffix());
    }
    శూన్య సందర్శించు(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), నిజం, "");
        puts(");\n");
    }
    శూన్య సందర్శించు(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), నిజం, "");
        puts(")");
    }
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), అబద్ధం, "");
        puts(");\n");
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_పాఠ్యం(nodep->nodesp(), అబద్ధం, "");
        puts(")");
    }

    శూన్య సందర్శించు(AstScopeName* nodep) override {}
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_తదుపరి_Const_Null(nodep->resultp());
        puts(");\n");
    }

    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    శూన్య సందర్శించు(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    మాయా శూన్య emitVerilogFormat(Ast_కణుపు* nodep, స్థిరరాశి string& format, Ast_కణుపు* lhsp = శూన్యసూచిక,
                                   Ast_కణుపు* స్థిరరాశి rhsp = శూన్యసూచిక, Ast_కణుపు* thsp = శూన్యసూచిక,
                                   Ast_కణుపు* fhsp = శూన్యసూచిక) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        తార్కిక లోపల_Pct = అబద్ధం;
        putbs("");
        కొరకు (స్థిరరాశి అక్షరం c : format) {
            ఒకవేళ (!లోపల_Pct && c == '%') {
                లోపల_Pct = నిజం;
            } లేకపోతే ఒకవేళ (!లోపల_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } లేకపోతే {  // Format character
                లోపల_Pct = అబద్ధం;
                ఎంపిక (c) {
                స్థితి '%': puts("%"); ఆపు;
                స్థితి 'f': putfs(nodep, ""); ఆపు;
                స్థితి 'k': putbs(""); ఆపు;
                స్థితి 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_తదుపరి_Const_Null(lhsp);
                    ఆపు;
                }
                స్థితి 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_తదుపరి_Const_Null(rhsp);
                    ఆపు;
                }
                స్థితి 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_తదుపరి_Const_Null(thsp);
                    ఆపు;
                }
                స్థితి 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_తదుపరి_Const_Null(fhsp);
                    ఆపు;
                }
                స్థితి 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_తదుపరి_Const_Null(nodep->dtypep());
                    ఆపు;
                }
                అప్రమేయ: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); ఆపు;
                }
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        ఒకవేళ (nodep->dimp()) {
            putbs(", ");
            iterate_And_తదుపరి_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    శూన్య సందర్శించు(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        కొరకు (స్థిరరాశి Ast_Cons_Array_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_అంశం)) {
            ఒకవేళ (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_గమ్యం_Str(itemp->సూచీ()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        putbs("(");
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_తదుపరి_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_తదుపరి_Const_Null(nodep->elsep());
        puts(")");
    }
    శూన్య సందర్శించు(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_తదుపరి_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_తదుపరి_Const_Null(nodep->rhsp());
        puts("]");
    }
    శూన్య సందర్శించు(AstRange* nodep) override {
        puts("[");
        ఒకవేళ (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_గమ్యం_Str(nodep->ఎడమ_Const()));
            puts(":");
            puts(cvt_గమ్యం_Str(nodep->కుడి_Const()));
        } లేకపోతే {
            iterate_And_తదుపరి_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_తదుపరి_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    శూన్య సందర్శించు(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        పూర్ణాంకం స్థానభ్రంశం = 0;
        Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->fromp()->dtypep();
        ఒకవేళ (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* స్థిరరాశి basicDtypep = VN_AS(dtypep, BasicDType);
            స్థానభ్రంశం = basicDtypep->lo();
        }
        puts("[");
        ఒకవేళ (VN_IS(nodep->lsbp(), Const)) {
            ఒకవేళ (nodep->వెడల్పు_Const() == 1) {
                puts(cvt_గమ్యం_Str(VN_AS(nodep->lsbp(), Const)->గమ్యం_SInt() + స్థానభ్రంశం));
            } లేకపోతే {
                puts(cvt_గమ్యం_Str(VN_AS(nodep->lsbp(), Const)->గమ్యం_SInt() + nodep->వెడల్పు_Const() + స్థానభ్రంశం
                              - 1));
                puts(":");
                puts(cvt_గమ్యం_Str(VN_AS(nodep->lsbp(), Const)->గమ్యం_SInt() + స్థానభ్రంశం));
            }
        } లేకపోతే {
            iterate_And_తదుపరి_Const_Null(nodep->lsbp());
            ఒకవేళ (స్థానభ్రంశం != 0) {
                puts(" + ");
                puts(cvt_గమ్యం_Str(స్థానభ్రంశం));
            }
            putfs(nodep, "+:");
            puts(cvt_గమ్యం_Str(nodep->వెడల్పు_Const()));
        }
        puts("]");
    }
    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        iterate_And_తదుపరి_Const_Null(nodep->fromp());
        puts(cvt_గమ్యం_Str(nodep->declRange()));
    }
    శూన్య సందర్శించు(AstThisRef* /*nodep*/) override { puts("this"); }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        ఒకవేళ (nodep->userType() == VAssertType::OBSERVED_DEFERRED_తక్షణ) {
            puts("#0 ");
        } లేకపోతే ఒకవేళ (nodep->userType() == VAssertType::FINAL_DEFERRED_తక్షణ) {
            puts("final ");
        } లేకపోతే ఒకవేళ (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        ఒకవేళ (AstSampled* స్థిరరాశి sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_తదుపరి_Const_Null(sampledp->exprp());
        } లేకపోతే {
            iterate_And_తదుపరి_Const_Null(nodep->propp());
        }
        ఒకవేళ (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_తదుపరి_Const_Null(nodep->passsp());
            puts("end\n");
        } లేకపోతే {
            puts(");\n");
        }

        ఒకవేళ (స్థిరరాశి AstAssert* స్థిరరాశి assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_తదుపరి_Const_Null(assertp->failsp());
            puts("end\n");
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstAssertIntrinsic* స్థిరరాశి assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_తదుపరి_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    శూన్య సందర్శించు(AstAssocArrayDType* nodep) override {
        ఒకవేళ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } లేకపోతే {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = అబద్ధం;
            puts("[");
            iterateConst(nodep->కీలకం_DTypep());
            puts("]");
            m_arrayPost = నిజం;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    శూన్య సందర్శించు(AstBasicDType* nodep) override {
        ఒకవేళ (m_arrayPost) తిరిగి;
        putfs(nodep, nodep->prettyName());
        ఒకవేళ (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        ఒకవేళ (nodep->keyword().isIntNumeric() && !nodep->keyword().is_బిట్టు_Logic()) తిరిగి;
        emitPacked();
        ఒకవేళ (nodep->rangep()) {
            puts(" ");
            iterate_And_తదుపరి_Const_Null(nodep->rangep());
            puts(" ");
        } లేకపోతే ఒకవేళ (nodep->isRanged()) {
            puts(" [");
            puts(cvt_గమ్యం_Str(nodep->ఎడమ()));
            puts(":");
            puts(cvt_గమ్యం_Str(nodep->కుడి()));
            puts("] ");
        }
    }
    శూన్య సందర్శించు(AstConstDType* nodep) override {
        ఒకవేళ (m_arrayPost) తిరిగి;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    శూన్య సందర్శించు(AstDynArrayDType* nodep) override {
        ఒకవేళ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } లేకపోతే {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    శూన్య సందర్శించు(AstEnumDType* nodep) override {
        ఒకవేళ (m_arrayPost) తిరిగి;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_తదుపరి_Const_Null(nodep->itemsp());
        puts("}");
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        ఒకవేళ (nodep->nextp()) puts(",");
        puts("\n");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Array_DType* nodep) override {
        ఒకవేళ (!m_arrayPost) {
            ఒకవేళ (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.నెట్టు_వెనుక(nodep);
            }
            iterateConst(nodep->subDTypep());
        } లేకపోతే {
            ఒకవేళ (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = అబద్ధం;
                iterate_And_తదుపరి_Const_Null(nodep->rangep());
                m_arrayPost = నిజం;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        ఒకవేళ (m_arrayPost) {
            puts(" (");
            ఒకవేళ (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } లేకపోతే {
                puts("????");
            }
            puts(")");
            తిరిగి;
        }
        puts(nodep->ifaceName());
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        ఒకవేళ (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } లేకపోతే {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::ఉపసర్గ_Name_Protect(nodep->classp()));
    }
    శూన్య సందర్శించు(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    శూన్య సందర్శించు(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        ఒకవేళ (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } లేకపోతే {
            puts("????");
        }
        puts(");\n");
    }
    శూన్య సందర్శించు(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        ఒకవేళ (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = నిజం;
            iterateConst(nodep->varp());
        } లేకపోతే {
            puts(nodep->prettyName());
        }
        ఒకవేళ (nodep->nextp()) puts(", ");
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        ఒకవేళ (m_arrayPost) తిరిగి;
        puts(nodep->verilogKwd() + " ");
        ఒకవేళ (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            కొరకు (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    శూన్య సందర్శించు(AstMemberDType* nodep) override {
        ఒకవేళ (m_arrayPost) తిరిగి;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    శూన్య సందర్శించు(Ast_వరుస_DType* nodep) override {
        ఒకవేళ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } లేకపోతే {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = అబద్ధం;
            puts("[$");
            ఒకవేళ (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = నిజం;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } లేకపోతే {
            putfs(nodep, nodep->prettyName());
        }
        ఒకవేళ (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_తదుపరి_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    శూన్య సందర్శించు(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_తదుపరి_Const_Null(nodep->argsp());
        puts(")");
    }
    శూన్య సందర్శించు(AstArg* nodep) override { iterate_And_తదుపరి_Const_Null(nodep->exprp()); }
    శూన్య సందర్శించు(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    శూన్య సందర్శించు(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    శూన్య సందర్శించు(Ast_Print_సమయం_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    శూన్య సందర్శించు(AstPropSpec* nodep) override {
        ఒకవేళ (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            కొరకు (Ast_కణుపు* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                ఒకవేళ (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        ఒకవేళ (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        ఒకవేళ (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        ఒకవేళ (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    శూన్య సందర్శించు(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    శూన్య సందర్శించు(AstAbortOn* nodep) override {
        puts(nodep->రకం().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    శూన్య సందర్శించు(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    శూన్య సందర్శించు(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        ఒకవేళ (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    శూన్య సందర్శించు(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = నిజం;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } లేకపోతే {
            ఒకవేళ (nodep->varp()) {
                ఒకవేళ (nodep->self_సూచిక().is_ఖాళీలు()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } లేకపోతే {
                    putfs(nodep, nodep->self_సూచిక().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } లేకపోతే {
                putfs(nodep, nodep->name());
            }
        }
    }
    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        ఒకవేళ (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } లేకపోతే {
            puts(nodep->prettyName());
        }
    }
    శూన్య సందర్శించు(AstConst* nodep) override { putfs(nodep, nodep->అంకె().ascii(m_prefixed, నిజం)); }
    శూన్య సందర్శించు(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    శూన్య సందర్శించు(Ast_శిఖరం_Scope* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(AstScope* nodep) override { iterateChildrenConst(nodep); }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = అబద్ధం;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = నిజం;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        ఒకవేళ (nodep->is_చక్రం_Delay()) {
            puts("##");
        } లేకపోతే {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = అబద్ధం;
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } లేకపోతే {
            iterateConst(nodep->lhsp());
        }
        ఒకవేళ (!m_suppressSemi) {
            puts(";\n");
        } లేకపోతే {
            puts(" ");
        }
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        ఒకవేళ (nodep->stmtsp()) {
            puts(" ");
            iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        } లేకపోతే {
            puts(";\n");
        }
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    శూన్య సందర్శించు(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    శూన్య సందర్శించు(AstVarScope*) override {}
    శూన్య సందర్శించు(AstTraceDecl*) override {}
    శూన్య సందర్శించు(AstTraceInc*) override {}
    // NOPs
    శూన్య సందర్శించు(AstPragma*) override {}
    శూన్య సందర్శించు(AstStmtPragma*) override {}
    శూన్య సందర్శించు(AstCell*) override {}  // Handled outside the Visit class
    // Default
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        ఒకవేళ (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

బహిరంగ:
    స్పష్టమైన EmitVBaseVisitorConst(తార్కిక suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = అప్రమేయ;
};

//######################################################################
// Emit to an output file

తరగతి EmitVFileVisitor final : బహిరంగ EmitVBaseVisitorConst {
    // STATE
    V3_బహిర్గతం_VFile& m_యొక్క;  // The output file
    // METHODS
    శూన్య putsNoTracking(స్థిరరాశి string& str) override { m_యొక్క.putsNoTracking(str); }
    శూన్య puts(స్థిరరాశి string& str) override { m_యొక్క.puts(str); }
    శూన్య putbs(స్థిరరాశి string& str) override { m_యొక్క.putbs(str); }
    శూన్య putfs(Ast_కణుపు*, స్థిరరాశి string& str) override { putbs(str); }
    శూన్య putqs(Ast_కణుపు*, స్థిరరాశి string& str) override { putbs(str); }

బహిరంగ:
    EmitVFileVisitor(Ast_కణుపు* nodep, V3_బహిర్గతం_VFile& యొక్క, తార్కిక suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_యొక్క{యొక్క} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = అప్రమేయ;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

తరగతి EmitVStreamVisitor final : బహిరంగ EmitVBaseVisitorConst {
    // STATE
    V3_బహిర్గతం_ప్రవాహం m_os;  // The output stream formatter
    స్థిరరాశి తార్కిక m_tracking;  // Use line tracking
    // METHODS
    శూన్య putsNoTracking(స్థిరరాశి string& str) override { m_os.putsNoTracking(str); }
    శూన్య puts(స్థిరరాశి string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    శూన్య putbs(స్థిరరాశి string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    శూన్య putfs(Ast_కణుపు*, స్థిరరాశి string& str) override { putbs(str); }
    శూన్య putqs(Ast_కణుపు*, స్థిరరాశి string& str) override { putbs(str); }

బహిరంగ:
    EmitVStreamVisitor(స్థిరరాశి Ast_కణుపు* nodep, std::ostream& os, తార్కిక tracking, తార్కిక suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_బహిర్గతం_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_కణుపు*>(nodep));
    }
    ~EmitVStreamVisitor() override = అప్రమేయ;
};

//######################################################################
// EmitV class functions

శూన్య V3EmitV::verilog_For_వృక్షం(స్థిరరాశి Ast_కణుపు* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ అబద్ధం, అబద్ధం}; }
}

శూన్య V3EmitV::debug_Verilog_For_వృక్షం(స్థిరరాశి Ast_కణుపు* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ నిజం, నిజం}; }
}

std::string V3EmitV::debug_Verilog_For_వృక్షం(స్థిరరాశి Ast_కణుపు* nodep) {
    std::stringstream ss;
    debug_Verilog_For_వృక్షం(nodep, ss);
    తిరిగి ss.str();
}

శూన్య V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    కొరకు (Ast_కణుపు_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), కణుపు_File)) {
        AstVFile* స్థిరరాశి vfilep = VN_CAST(filep, VFile);
        ఒకవేళ (vfilep && vfilep->tblockp()) {
            V3_బహిర్గతం_VFile యొక్క{vfilep->name()};
            యొక్క.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), యొక్క, అబద్ధం};
        }
    }
}

శూన్య V3EmitV::debugEmitV(స్థిరరాశి string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_బహిర్గతం_VFile యొక్క{filename};
    EmitVFileVisitor{v3Global.rootp(), యొక్క, నిజం};
}
