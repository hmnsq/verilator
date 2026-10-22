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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3EmitV.h"

#অন্তর্ভাব "V3EmitCBase.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

শ্রেণী EmitVBaseVisitorConst VL_NOT_FINAL : সর্বজনীন VNVisitorConst {
    // STATE - across all visitors
    ধ্রুবক যৌক্তিক m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_বৃক্ষ* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    যৌক্তিক m_suppressSemi = মিথ্যা;  // Non-statement, don't print ;
    যৌক্তিক m_suppressVarSemi = মিথ্যা;  // Suppress emitting semicolon for AstVars
    যৌক্তিক m_suppressSampled = মিথ্যা;  // Suppress emitting sampled in assertion properties
    যৌক্তিক m_arrayPost = মিথ্যা;  // Print array information that goes after identifier (vs after)
    যৌক্তিক m_prefixed = সত্য;  // Whether constants need to be prefixed
    std::deque<Ast_গ্রন্থি_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, আকার_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    আভাসী শূন্য puts(ধ্রুবক string& str) = 0;
    আভাসী শূন্য putbs(ধ্রুবক string& str) = 0;
    আভাসী শূন্য putfs(Ast_গ্রন্থি* nodep, ধ্রুবক string& str) = 0;  // Fileline and node %% mark
    আভাসী শূন্য putqs(Ast_গ্রন্থি* nodep, ধ্রুবক string& str) = 0;  // Fileline quiet w/o %% mark
    আভাসী শূন্য putsNoTracking(ধ্রুবক string& str) = 0;
    আভাসী শূন্য putsQuoted(ধ্রুবক string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    শূন্য iterateAndCommaConstNull(Ast_গ্রন্থি* nodep) {
        জন্য (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            যদি (nodep->nextp()) puts(", ");
        }
    }
    শূন্য emitPacked() {
        জন্য (Ast_গ্রন্থি_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    শূন্য emitNodesWithText(Ast_গ্রন্থি* nodesp, যৌক্তিক tracking, ধ্রুবক std::string& separator) {
        জন্য (Ast_গ্রন্থি* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            যদি (ধ্রুবক AstText* ধ্রুবক textp = VN_CAST(nodep, Text)) {
                যদি (tracking) {
                    puts(textp->text());
                } অন্যথা {
                    putsNoTracking(textp->text());
                }
            } অন্যথা {
                iterateConst(nodep);
            }
            যদি (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override { iterate_And_পরবর্তী_Const_Null(nodep->modulesp()); }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    শূন্য visit(AstPort* nodep) override {}
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    শূন্য visit(AstGenBlock* nodep) override {
        ধ্রুবক std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    শূন্য visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_পরবর্তী_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    শূন্য visit(AstGenCaseItem* nodep) override {
        যদি (nodep->condsp()) {
            iterate_And_পরবর্তী_Const_Null(nodep->condsp());
        } অন্যথা {
            putbs("default");
        }
        iterate_And_পরবর্তী_Const_Null(nodep->itemsp());
    }
    শূন্য visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = সত্য;
            iterate_And_পরবর্তী_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_পরবর্তী_Const_Null(nodep->condp());
            puts(";");
            iterate_And_পরবর্তী_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->thensp());
        যদি (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_পরবর্তী_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstBegin* nodep) override {
        যদি (nodep->name() == "") {
            putbs("begin\n");
        } অন্যথা {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (nodep->name() == "") {
            putbs("fork\n");
        } অন্যথা {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    শূন্য visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstAlways* nodep) override {
        যদি (ধ্রুবক AstAssignW* ধ্রুবক ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            যদি (!ap->nextp()) {
                putfs(nodep, "assign ");
                যদি (Ast_গ্রন্থি* ধ্রুবক tcp = ap->timingControlp()) {
                    iterate_And_পরবর্তী_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_পরবর্তী_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_পরবর্তী_Const_Null(ap->rhsp());
                যদি (!m_suppressSemi) puts(";\n");
                ফেরত;
            }
        }
        putfs(nodep, "always ");
        যদি (m_sentreep) {
            iterate_And_পরবর্তী_Const_Null(m_sentreep);
        }  // In active
        অন্যথা {
            iterate_And_পরবর্তী_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (VN_IS(nodep, AssignForce)) puts("force ");
        iterate_And_পরবর্তী_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_পরবর্তী_Const_Null(nodep->rhsp());
        যদি (!m_suppressSemi) puts(";\n");
    }
    শূন্য visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        জন্য (Ast_গ্রন্থি* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        যদি (!m_suppressSemi) puts(";\n");
    }
    শূন্য visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_পরবর্তী_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_পরবর্তী_Const_Null(nodep->rhsp());
        যদি (!m_suppressSemi) puts(";\n");
    }
    শূন্য visit(AstRelease* nodep) override {
        puts("release ");
        iterate_And_পরবর্তী_Const_Null(nodep->lhsp());
        যদি (!m_suppressSemi) puts(";\n");
    }
    শূন্য visit(AstBreak*) override {
        putbs("break");
        যদি (!m_suppressSemi) puts(";\n");
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        জন্য (Ast_গ্রন্থি* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            যদি (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    শূন্য visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        যদি (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        যদি (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    শূন্য visit(AstCase* nodep) override {
        putfs(nodep, "");
        যদি (nodep->priorityPragma()) puts("priority ");
        যদি (nodep->uniquePragma()) puts("unique ");
        যদি (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        puts(")\n");
        যদি (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            যদি (nodep->fullPragma()) puts(" full_case");
            যদি (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_পরবর্তী_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    শূন্য visit(AstCaseItem* nodep) override {
        যদি (nodep->condsp()) {
            iterate_And_পরবর্তী_Const_Null(nodep->condsp());
        } অন্যথা {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstContinue*) override {
        putbs("continue");
        যদি (!m_suppressSemi) puts(";\n");
    }
    শূন্য visit(Ast_গ্রন্থি_Cover_Decl*) override {}  // N/A
    শূন্য visit(AstCoverInc*) override {}  // N/A
    শূন্য visit(AstCoverToggle*) override {}  // N/A

    শূন্য visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        যদি (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    শূন্য visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        যদি (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_পরবর্তী_Const_Null(nodep->optionsp());
            iterate_And_পরবর্তী_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    শূন্য emitCoverageOption(Ast_গ্রন্থি* nodep, যৌক্তিক typeOption, VCoverOptionType optType,
                            Ast_গ্রন্থি_রাশিমালা* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    শূন্য visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    শূন্য visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    শূন্য visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        যদি (nodep->isArray()) {
            puts("[");
            যদি (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        যদি (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } অন্যথা যদি (nodep->transp()) {
            puts(" = ");
            জন্য (Ast_গ্রন্থি* setp = nodep->transp(); setp; setp = setp->nextp()) {
                যদি (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } অন্যথা যদি (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } অন্যথা যদি (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            জন্য (Ast_গ্রন্থি* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                যদি (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        যদি (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    শূন্য visit(AstCoverWith* nodep) override {
        ধ্রুবক যৌক্তিক rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        যদি (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        যদি (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    শূন্য visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        যদি (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        যদি (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    শূন্য visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        যদি (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    শূন্য visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    শূন্য visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    শূন্য visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        যদি (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        যদি (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    শূন্য visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    শূন্য visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        জন্য (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        যদি (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        যদি (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_পরবর্তী_Const_Null(nodep->optionsp());
            iterate_And_পরবর্তী_Const_Null(nodep->binsp());
            puts("}\n");
        } অন্যথা {
            puts(";\n");
        }
    }
    শূন্য visit(AstCoverTransSet* nodep) override {
        puts("(");
        জন্য (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    শূন্য visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    শূন্য visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        যদি (AstConst* ধ্রুবক lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->অঙ্ক().toString());
        } অন্যথা {
            puts("string'(");
            iterate_And_পরবর্তী_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    শূন্য visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    শূন্য visit(Ast_মান_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    শূন্য visit_গ্রন্থি_Display(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* fileOrStrgp, ধ্রুবক string& text,
                          Ast_গ্রন্থি* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        যদি (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        জন্য (Ast_গ্রন্থি* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    শূন্য visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    শূন্য visit(AstDisplay* nodep) override {
        visit_গ্রন্থি_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    শূন্য visit(AstElabDisplay* nodep) override {
        visit_গ্রন্থি_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    শূন্য visit(AstFScanF* nodep) override {
        visit_গ্রন্থি_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    শূন্য visit(AstSScanF* nodep) override {
        visit_গ্রন্থি_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    শূন্য visit(AstSFormat* nodep) override {
        visit_গ্রন্থি_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    শূন্য visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    শূন্য visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    শূন্য visit(AstSFormatF* nodep) override {
        visit_গ্রন্থি_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    শূন্য visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_পরবর্তী_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_পরবর্তী_Const_Null(nodep->modep());
        puts(");\n");
    }
    শূন্য visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_পরবর্তী_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    শূন্য visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        যদি (nodep->filep()) iterate_And_পরবর্তী_Const_Null(nodep->filep());
        puts(");\n");
    }
    শূন্য visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        যদি (nodep->filep()) iterate_And_পরবর্তী_Const_Null(nodep->filep());
        puts(");\n");
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ধ্রুবক আকার_t n = m_labelNumbers.আকার_2();
        ধ্রুবক যৌক্তিক newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    শূন্য visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ধ্রুবক auto it = m_labelNumbers.find(nodep->blockp());
        ধ্রুবক std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    শূন্য visit(Ast_গ্রন্থি_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        যদি (nodep->filenamep()) iterate_And_পরবর্তী_Const_Null(nodep->filenamep());
        putbs(", ");
        যদি (nodep->memp()) iterate_And_পরবর্তী_Const_Null(nodep->memp());
        যদি (nodep->lsbp()) {
            putbs(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->lsbp());
        }
        যদি (nodep->msbp()) {
            putbs(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    শূন্য visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    শূন্য visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_পরবর্তী_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    শূন্য visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        যদি (AstLoopTest* ধ্রুবক testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_পরবর্তী_Const_Null(testp->nextp());
            puts("end\n");
            ফেরত;
        }
        // Special case when the AstLoopTest is last for output readability
        যদি (Ast_গ্রন্থি* lastp = nodep->stmtsp()) {
            যখন (Ast_গ্রন্থি* ধ্রুবক nextp = lastp->nextp()) lastp = nextp;
            যদি (AstLoopTest* ধ্রুবক testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                জন্য (Ast_গ্রন্থি* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                ফেরত;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        iterate_And_পরবর্তী_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    শূন্য visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        putfs(nodep, "");
        যদি (ধ্রুবক AstIf* ধ্রুবক ifp = VN_CAST(nodep, If)) {
            যদি (ifp->priorityPragma()) puts("priority ");
            যদি (ifp->uniquePragma()) puts("unique ");
            যদি (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_পরবর্তী_Const_Null(nodep->thensp());
        যদি (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_পরবর্তী_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    শূন্য visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        যদি (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->ticksp());
            যদি (nodep->sentreep()) {
                puts(", ");
                iterate_And_পরবর্তী_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    শূন্য visit(AstSampled* nodep) override {
        যদি (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_পরবর্তী_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    শূন্য visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        puts(")");
    }
    শূন্য visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        যদি (nodep->sentreep()) {
            puts(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    শূন্য visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        যদি (nodep->sentreep()) {
            puts(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    শূন্য visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        puts(")");
    }
    শূন্য visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        puts(")");
    }
    শূন্য visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        যদি (nodep->sentreep()) {
            puts(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    শূন্য visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
        puts(")");
    }
    শূন্য visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_পরবর্তী_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    শূন্য visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    শূন্য visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    শূন্য visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    শূন্য visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    শূন্য visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), সত্য, nodep->separator());
        puts(nodep->suffix());
    }
    শূন্য visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), সত্য, "");
        puts(");\n");
    }
    শূন্য visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), সত্য, "");
        puts(")");
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), মিথ্যা, "");
        puts(");\n");
    }
    শূন্য visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), মিথ্যা, "");
        puts(")");
    }

    শূন্য visit(AstScopeName* nodep) override {}
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_পরবর্তী_Const_Null(nodep->resultp());
        puts(");\n");
    }

    শূন্য visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    শূন্য visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    আভাসী শূন্য emitVerilogFormat(Ast_গ্রন্থি* nodep, ধ্রুবক string& format, Ast_গ্রন্থি* lhsp = nullptr,
                                   Ast_গ্রন্থি* ধ্রুবক rhsp = nullptr, Ast_গ্রন্থি* thsp = nullptr,
                                   Ast_গ্রন্থি* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        যৌক্তিক inPct = মিথ্যা;
        putbs("");
        জন্য (ধ্রুবক অক্ষর c : format) {
            যদি (!inPct && c == '%') {
                inPct = সত্য;
            } অন্যথা যদি (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } অন্যথা {  // Format character
                inPct = মিথ্যা;
                নির্বাচন (c) {
                ক্ষেত্র '%': puts("%"); বিরতি;
                ক্ষেত্র 'f': putfs(nodep, ""); বিরতি;
                ক্ষেত্র 'k': putbs(""); বিরতি;
                ক্ষেত্র 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_পরবর্তী_Const_Null(lhsp);
                    বিরতি;
                }
                ক্ষেত্র 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_পরবর্তী_Const_Null(rhsp);
                    বিরতি;
                }
                ক্ষেত্র 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_পরবর্তী_Const_Null(thsp);
                    বিরতি;
                }
                ক্ষেত্র 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_পরবর্তী_Const_Null(fhsp);
                    বিরতি;
                }
                ক্ষেত্র 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_পরবর্তী_Const_Null(nodep->dtypep());
                    বিরতি;
                }
                পূর্বনির্ধারিত: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); বিরতি;
                }
            }
        }
    }

    শূন্য visit(Ast_গ্রন্থি_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    শূন্য visit(Ast_গ্রন্থি_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    শূন্য visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    শূন্য visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    শূন্য visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_পরবর্তী_Const_Null(nodep->fromp());
        যদি (nodep->dimp()) {
            putbs(", ");
            iterate_And_পরবর্তী_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    শূন্য visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        জন্য (ধ্রুবক AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            যদি (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->সূচক()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    শূন্য visit(AstCond* nodep) override {
        putbs("(");
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_পরবর্তী_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_পরবর্তী_Const_Null(nodep->elsep());
        puts(")");
    }
    শূন্য visit(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_পরবর্তী_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_পরবর্তী_Const_Null(nodep->rhsp());
        puts("]");
    }
    শূন্য visit(AstRange* nodep) override {
        puts("[");
        যদি (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->বাম_Const()));
            puts(":");
            puts(cvtToStr(nodep->ডান_Const()));
        } অন্যথা {
            iterate_And_পরবর্তী_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_পরবর্তী_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    শূন্য visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    শূন্য visit(AstSel* nodep) override {
        iterate_And_পরবর্তী_Const_Null(nodep->fromp());
        পূর্ণসংখ্যা offset = 0;
        Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->fromp()->dtypep();
        যদি (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ধ্রুবক basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        যদি (VN_IS(nodep->lsbp(), Const)) {
            যদি (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } অন্যথা {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } অন্যথা {
            iterate_And_পরবর্তী_Const_Null(nodep->lsbp());
            যদি (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    শূন্য visit(AstSliceSel* nodep) override {
        iterate_And_পরবর্তী_Const_Null(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    শূন্য visit(AstThisRef* /*nodep*/) override { puts("this"); }
    শূন্য visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    শূন্য visit(Ast_গ্রন্থি_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        যদি (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } অন্যথা যদি (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } অন্যথা যদি (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        যদি (AstSampled* ধ্রুবক sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_পরবর্তী_Const_Null(sampledp->exprp());
        } অন্যথা {
            iterate_And_পরবর্তী_Const_Null(nodep->propp());
        }
        যদি (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_পরবর্তী_Const_Null(nodep->passsp());
            puts("end\n");
        } অন্যথা {
            puts(");\n");
        }

        যদি (ধ্রুবক AstAssert* ধ্রুবক assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_পরবর্তী_Const_Null(assertp->failsp());
            puts("end\n");
        } অন্যথা যদি (ধ্রুবক AstAssertIntrinsic* ধ্রুবক assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_পরবর্তী_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    শূন্য visit(AstAssocArrayDType* nodep) override {
        যদি (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } অন্যথা {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = মিথ্যা;
            puts("[");
            iterateConst(nodep->চাবি_DTypep());
            puts("]");
            m_arrayPost = সত্য;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    শূন্য visit(AstBasicDType* nodep) override {
        যদি (m_arrayPost) ফেরত;
        putfs(nodep, nodep->prettyName());
        যদি (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        যদি (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) ফেরত;
        emitPacked();
        যদি (nodep->rangep()) {
            puts(" ");
            iterate_And_পরবর্তী_Const_Null(nodep->rangep());
            puts(" ");
        } অন্যথা যদি (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->বাম()));
            puts(":");
            puts(cvtToStr(nodep->ডান()));
            puts("] ");
        }
    }
    শূন্য visit(AstConstDType* nodep) override {
        যদি (m_arrayPost) ফেরত;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    শূন্য visit(AstDynArrayDType* nodep) override {
        যদি (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } অন্যথা {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    শূন্য visit(AstEnumDType* nodep) override {
        যদি (m_arrayPost) ফেরত;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_পরবর্তী_Const_Null(nodep->itemsp());
        puts("}");
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        যদি (Ast_গ্রন্থি_Module* ধ্রুবক classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    শূন্য visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        যদি (nodep->nextp()) puts(",");
        puts("\n");
    }
    শূন্য visit(Ast_গ্রন্থি_Array_DType* nodep) override {
        যদি (!m_arrayPost) {
            যদি (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ঠেলো_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } অন্যথা {
            যদি (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = মিথ্যা;
                iterate_And_পরবর্তী_Const_Null(nodep->rangep());
                m_arrayPost = সত্য;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    শূন্য visit(AstIfaceRefDType* nodep) override {
        যদি (m_arrayPost) {
            puts(" (");
            যদি (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } অন্যথা {
                puts("????");
            }
            puts(")");
            ফেরত;
        }
        puts(nodep->ifaceName());
    }
    শূন্য visit(AstRefDType* nodep) override {
        যদি (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } অন্যথা {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    শূন্য visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    শূন্য visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        যদি (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } অন্যথা {
            puts("????");
        }
        puts(");\n");
    }
    শূন্য visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        যদি (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = সত্য;
            iterateConst(nodep->varp());
        } অন্যথা {
            puts(nodep->prettyName());
        }
        যদি (nodep->nextp()) puts(", ");
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        যদি (m_arrayPost) ফেরত;
        puts(nodep->verilogKwd() + " ");
        যদি (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            জন্য (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    শূন্য visit(AstMemberDType* nodep) override {
        যদি (m_arrayPost) ফেরত;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    শূন্য visit(AstQueueDType* nodep) override {
        যদি (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } অন্যথা {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = মিথ্যা;
            puts("[$");
            যদি (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = সত্য;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        যদি (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } অন্যথা {
            putfs(nodep, nodep->prettyName());
        }
        যদি (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_পরবর্তী_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    শূন্য visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_পরবর্তী_Const_Null(nodep->argsp());
        puts(")");
    }
    শূন্য visit(AstArg* nodep) override { iterate_And_পরবর্তী_Const_Null(nodep->exprp()); }
    শূন্য visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    শূন্য visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    শূন্য visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    শূন্য visit(AstPropSpec* nodep) override {
        যদি (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            জন্য (Ast_গ্রন্থি* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                যদি (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        যদি (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        যদি (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        যদি (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    শূন্য visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    শূন্য visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    শূন্য visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    শূন্য visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        যদি (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    শূন্য visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = সত্য;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    শূন্য visit(AstVarRef* nodep) override {
        যদি (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } অন্যথা {
            যদি (nodep->varp()) {
                যদি (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } অন্যথা {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } অন্যথা {
                putfs(nodep, nodep->name());
            }
        }
    }
    শূন্য visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        যদি (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } অন্যথা {
            puts(nodep->prettyName());
        }
    }
    শূন্য visit(AstConst* nodep) override { putfs(nodep, nodep->অঙ্ক().ascii(m_prefixed, সত্য)); }
    শূন্য visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    শূন্য visit(Ast_শীর্ষ_Scope* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    শূন্য visit(AstVar* nodep) override {
        যদি (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = মিথ্যা;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = সত্য;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    শূন্য visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
    }
    শূন্য visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        যদি (nodep->isCycleDelay()) {
            puts("##");
        } অন্যথা {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = মিথ্যা;
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } অন্যথা {
            iterateConst(nodep->lhsp());
        }
        যদি (!m_suppressSemi) {
            puts(";\n");
        } অন্যথা {
            puts(" ");
        }
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
    }
    শূন্য visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        যদি (nodep->stmtsp()) {
            puts(" ");
            iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        } অন্যথা {
            puts(";\n");
        }
    }
    শূন্য visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    শূন্য visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    শূন্য visit(AstVarScope*) override {}
    শূন্য visit(AstTraceDecl*) override {}
    শূন্য visit(AstTraceInc*) override {}
    // NOPs
    শূন্য visit(AstPragma*) override {}
    শূন্য visit(AstStmtPragma*) override {}
    শূন্য visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        যদি (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

সর্বজনীন:
    explicit EmitVBaseVisitorConst(যৌক্তিক suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = পূর্বনির্ধারিত;
};

//######################################################################
// Emit to an output file

শ্রেণী EmitVFileVisitor final : সর্বজনীন EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    শূন্য putsNoTracking(ধ্রুবক string& str) override { m_of.putsNoTracking(str); }
    শূন্য puts(ধ্রুবক string& str) override { m_of.puts(str); }
    শূন্য putbs(ধ্রুবক string& str) override { m_of.putbs(str); }
    শূন্য putfs(Ast_গ্রন্থি*, ধ্রুবক string& str) override { putbs(str); }
    শূন্য putqs(Ast_গ্রন্থি*, ধ্রুবক string& str) override { putbs(str); }

সর্বজনীন:
    EmitVFileVisitor(Ast_গ্রন্থি* nodep, V3OutVFile& of, যৌক্তিক suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

শ্রেণী EmitVStreamVisitor final : সর্বজনীন EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    ধ্রুবক যৌক্তিক m_tracking;  // Use line tracking
    // METHODS
    শূন্য putsNoTracking(ধ্রুবক string& str) override { m_os.putsNoTracking(str); }
    শূন্য puts(ধ্রুবক string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    শূন্য putbs(ধ্রুবক string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    শূন্য putfs(Ast_গ্রন্থি*, ধ্রুবক string& str) override { putbs(str); }
    শূন্য putqs(Ast_গ্রন্থি*, ধ্রুবক string& str) override { putbs(str); }

সর্বজনীন:
    EmitVStreamVisitor(ধ্রুবক Ast_গ্রন্থি* nodep, std::ostream& os, যৌক্তিক tracking, যৌক্তিক suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_গ্রন্থি*>(nodep));
    }
    ~EmitVStreamVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// EmitV class functions

শূন্য V3EmitV::verilog_For_বৃক্ষ(ধ্রুবক Ast_গ্রন্থি* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ মিথ্যা, মিথ্যা}; }
}

শূন্য V3EmitV::debug_Verilog_For_বৃক্ষ(ধ্রুবক Ast_গ্রন্থি* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ সত্য, সত্য}; }
}

std::string V3EmitV::debug_Verilog_For_বৃক্ষ(ধ্রুবক Ast_গ্রন্থি* nodep) {
    std::stringstream ss;
    debug_Verilog_For_বৃক্ষ(nodep, ss);
    ফেরত ss.str();
}

শূন্য V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    জন্য (Ast_গ্রন্থি_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), গ্রন্থি_File)) {
        AstVFile* ধ্রুবক vfilep = VN_CAST(filep, VFile);
        যদি (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, মিথ্যা};
        }
    }
}

শূন্য V3EmitV::debugEmitV(ধ্রুবক string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, সত্য};
}
