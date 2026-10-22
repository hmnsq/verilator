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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3EmitV.h"

#அந்தர்பாவ "V3EmitCBase.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

வகுப்பு EmitVBaseVisitorConst VL_NOT_FINAL : பொது VNVisitorConst {
    // STATE - across all visitors
    மாறிலி தருக்க m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_மரம்* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    தருக்க m_suppressSemi = பொய்;  // Non-statement, don't print ;
    தருக்க m_suppressVarSemi = பொய்;  // Suppress emitting semicolon for AstVars
    தருக்க m_suppressSampled = பொய்;  // Suppress emitting sampled in assertion properties
    தருக்க m_arrayPost = பொய்;  // Print array information that goes after identifier (vs after)
    தருக்க m_prefixed = உண்மை;  // Whether constants need to be prefixed
    std::deque<Ast_முனை_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    மெய்நிகர் வெறுமை puts(மாறிலி string& str) = 0;
    மெய்நிகர் வெறுமை putbs(மாறிலி string& str) = 0;
    மெய்நிகர் வெறுமை putfs(Ast_முனை* nodep, மாறிலி string& str) = 0;  // Fileline and node %% mark
    மெய்நிகர் வெறுமை putqs(Ast_முனை* nodep, மாறிலி string& str) = 0;  // Fileline quiet w/o %% mark
    மெய்நிகர் வெறுமை putsNoTracking(மாறிலி string& str) = 0;
    மெய்நிகர் வெறுமை putsQuoted(மாறிலி string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    வெறுமை iterateAndCommaConstNull(Ast_முனை* nodep) {
        க்கு (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            எனில் (nodep->nextp()) puts(", ");
        }
    }
    வெறுமை emitPacked() {
        க்கு (Ast_முனை_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    வெறுமை emitNodesWithText(Ast_முனை* nodesp, தருக்க tracking, மாறிலி std::string& separator) {
        க்கு (Ast_முனை* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            எனில் (மாறிலி AstText* மாறிலி textp = VN_CAST(nodep, Text)) {
                எனில் (tracking) {
                    puts(textp->text());
                } இல்லையெனில் {
                    putsNoTracking(textp->text());
                }
            } இல்லையெனில் {
                iterateConst(nodep);
            }
            எனில் (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    வெறுமை visit(AstPort* nodep) override {}
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    வெறுமை visit(AstGenBlock* nodep) override {
        மாறிலி std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    வெறுமை visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    வெறுமை visit(AstGenCaseItem* nodep) override {
        எனில் (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } இல்லையெனில் {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    வெறுமை visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = உண்மை;
            iterateAndNextConstNull(nodep->initsp());
            puts(";");
            iterateAndNextConstNull(nodep->condp());
            puts(";");
            iterateAndNextConstNull(nodep->incsp());
        }
        puts(") begin\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        எனில் (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstBegin* nodep) override {
        எனில் (nodep->name() == "") {
            putbs("begin\n");
        } இல்லையெனில் {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    வெறுமை visit(AstFork* nodep) override {
        எனில் (nodep->name() == "") {
            putbs("fork\n");
        } இல்லையெனில் {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    வெறுமை visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (மாறிலி AstAssignW* மாறிலி ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            எனில் (!ap->nextp()) {
                putfs(nodep, "assign ");
                எனில் (Ast_முனை* மாறிலி tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                எனில் (!m_suppressSemi) puts(";\n");
                திரும்பு;
            }
        }
        putfs(nodep, "always ");
        எனில் (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        இல்லையெனில் {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        எனில் (!m_suppressSemi) puts(";\n");
    }
    வெறுமை visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        க்கு (Ast_முனை* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        எனில் (!m_suppressSemi) puts(";\n");
    }
    வெறுமை visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        எனில் (!m_suppressSemi) puts(";\n");
    }
    வெறுமை visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        எனில் (!m_suppressSemi) puts(";\n");
    }
    வெறுமை visit(AstBreak*) override {
        putbs("break");
        எனில் (!m_suppressSemi) puts(";\n");
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        க்கு (Ast_முனை* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            எனில் (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    வெறுமை visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        எனில் (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        எனில் (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    வெறுமை visit(AstCase* nodep) override {
        putfs(nodep, "");
        எனில் (nodep->priorityPragma()) puts("priority ");
        எனில் (nodep->uniquePragma()) puts("unique ");
        எனில் (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        எனில் (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            எனில் (nodep->fullPragma()) puts(" full_case");
            எனில் (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        எனில் (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } இல்லையெனில் {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstContinue*) override {
        putbs("continue");
        எனில் (!m_suppressSemi) puts(";\n");
    }
    வெறுமை visit(Ast_முனை_Cover_Decl*) override {}  // N/A
    வெறுமை visit(AstCoverInc*) override {}  // N/A
    வெறுமை visit(AstCoverToggle*) override {}  // N/A

    வெறுமை visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        எனில் (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    வெறுமை visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        எனில் (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    வெறுமை emitCoverageOption(Ast_முனை* nodep, தருக்க typeOption, VCoverOptionType optType,
                            Ast_முனை_கோவை* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    வெறுமை visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    வெறுமை visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    வெறுமை visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        எனில் (nodep->isArray()) {
            puts("[");
            எனில் (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        எனில் (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } இல்லையெனில் எனில் (nodep->transp()) {
            puts(" = ");
            க்கு (Ast_முனை* setp = nodep->transp(); setp; setp = setp->nextp()) {
                எனில் (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } இல்லையெனில் எனில் (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } இல்லையெனில் எனில் (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            க்கு (Ast_முனை* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                எனில் (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        எனில் (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    வெறுமை visit(AstCoverWith* nodep) override {
        மாறிலி தருக்க rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        எனில் (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        எனில் (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    வெறுமை visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        எனில் (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        எனில் (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    வெறுமை visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        எனில் (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    வெறுமை visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    வெறுமை visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    வெறுமை visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        எனில் (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        எனில் (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    வெறுமை visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    வெறுமை visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        க்கு (Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            எனில் (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        எனில் (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        எனில் (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } இல்லையெனில் {
            puts(";\n");
        }
    }
    வெறுமை visit(AstCoverTransSet* nodep) override {
        puts("(");
        க்கு (Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            எனில் (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    வெறுமை visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    வெறுமை visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        எனில் (AstConst* மாறிலி lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->இலக்கம்().toString());
        } இல்லையெனில் {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    வெறுமை visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    வெறுமை visit(Ast_மதிப்பு_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    வெறுமை visit_முனை_Display(Ast_முனை* nodep, Ast_முனை* fileOrStrgp, மாறிலி string& text,
                          Ast_முனை* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        எனில் (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        க்கு (Ast_முனை* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    வெறுமை visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    வெறுமை visit(AstDisplay* nodep) override {
        visit_முனை_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    வெறுமை visit(AstElabDisplay* nodep) override {
        visit_முனை_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    வெறுமை visit(AstFScanF* nodep) override {
        visit_முனை_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    வெறுமை visit(AstSScanF* nodep) override {
        visit_முனை_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    வெறுமை visit(AstSFormat* nodep) override {
        visit_முனை_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    வெறுமை visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    வெறுமை visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    வெறுமை visit(AstSFormatF* nodep) override {
        visit_முனை_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    வெறுமை visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    வெறுமை visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    வெறுமை visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        எனில் (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    வெறுமை visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        எனில் (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    வெறுமை visit(AstJumpBlock* nodep) override {
        // Allocate label number
        மாறிலி size_t n = m_labelNumbers.size();
        மாறிலி தருக்க newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    வெறுமை visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        மாறிலி auto it = m_labelNumbers.find(nodep->blockp());
        மாறிலி std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    வெறுமை visit(Ast_முனை_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        எனில் (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        எனில் (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        எனில் (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        எனில் (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    வெறுமை visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    வெறுமை visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    வெறுமை visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        எனில் (AstLoopTest* மாறிலி testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            திரும்பு;
        }
        // Special case when the AstLoopTest is last for output readability
        எனில் (Ast_முனை* lastp = nodep->stmtsp()) {
            போது (Ast_முனை* மாறிலி nextp = lastp->nextp()) lastp = nextp;
            எனில் (AstLoopTest* மாறிலி testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                க்கு (Ast_முனை* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                திரும்பு;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        putfs(nodep, "");
        எனில் (மாறிலி AstIf* மாறிலி ifp = VN_CAST(nodep, If)) {
            எனில் (ifp->priorityPragma()) puts("priority ");
            எனில் (ifp->uniquePragma()) puts("unique ");
            எனில் (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        எனில் (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    வெறுமை visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        எனில் (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            எனில் (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    வெறுமை visit(AstSampled* nodep) override {
        எனில் (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    வெறுமை visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    வெறுமை visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        எனில் (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    வெறுமை visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        எனில் (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    வெறுமை visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    வெறுமை visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    வெறுமை visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        எனில் (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    வெறுமை visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    வெறுமை visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    வெறுமை visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    வெறுமை visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    வெறுமை visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    வெறுமை visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    வெறுமை visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), உண்மை, nodep->separator());
        puts(nodep->suffix());
    }
    வெறுமை visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), உண்மை, "");
        puts(");\n");
    }
    வெறுமை visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), உண்மை, "");
        puts(")");
    }
    வெறுமை visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), பொய், "");
        puts(");\n");
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), பொய், "");
        puts(")");
    }

    வெறுமை visit(AstScopeName* nodep) override {}
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    வெறுமை visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    வெறுமை visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    மெய்நிகர் வெறுமை emitVerilogFormat(Ast_முனை* nodep, மாறிலி string& format, Ast_முனை* lhsp = nullptr,
                                   Ast_முனை* மாறிலி rhsp = nullptr, Ast_முனை* thsp = nullptr,
                                   Ast_முனை* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        தருக்க inPct = பொய்;
        putbs("");
        க்கு (மாறிலி எழுத்து c : format) {
            எனில் (!inPct && c == '%') {
                inPct = உண்மை;
            } இல்லையெனில் எனில் (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } இல்லையெனில் {  // Format character
                inPct = பொய்;
                தேர்வு (c) {
                நிலை '%': puts("%"); நிறுத்து;
                நிலை 'f': putfs(nodep, ""); நிறுத்து;
                நிலை 'k': putbs(""); நிறுத்து;
                நிலை 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    நிறுத்து;
                }
                நிலை 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    நிறுத்து;
                }
                நிலை 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    நிறுத்து;
                }
                நிலை 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    நிறுத்து;
                }
                நிலை 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    நிறுத்து;
                }
                இயல்பு: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); நிறுத்து;
                }
            }
        }
    }

    வெறுமை visit(Ast_முனை_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    வெறுமை visit(Ast_முனை_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    வெறுமை visit(Ast_முனை_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    வெறுமை visit(Ast_முனை_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    வெறுமை visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    வெறுமை visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        எனில் (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    வெறுமை visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        க்கு (மாறிலி AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            எனில் (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    வெறுமை visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    வெறுமை visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    வெறுமை visit(AstRange* nodep) override {
        puts("[");
        எனில் (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->இடது_Const()));
            puts(":");
            puts(cvtToStr(nodep->வலது_Const()));
        } இல்லையெனில் {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    வெறுமை visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    வெறுமை visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        முழு offset = 0;
        Ast_முனை_DType* மாறிலி dtypep = nodep->fromp()->dtypep();
        எனில் (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* மாறிலி basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        எனில் (VN_IS(nodep->lsbp(), Const)) {
            எனில் (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } இல்லையெனில் {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } இல்லையெனில் {
            iterateAndNextConstNull(nodep->lsbp());
            எனில் (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    வெறுமை visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    வெறுமை visit(AstThisRef* /*nodep*/) override { puts("this"); }
    வெறுமை visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    வெறுமை visit(Ast_முனை_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        எனில் (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } இல்லையெனில் எனில் (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } இல்லையெனில் எனில் (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        எனில் (AstSampled* மாறிலி sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } இல்லையெனில் {
            iterateAndNextConstNull(nodep->propp());
        }
        எனில் (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } இல்லையெனில் {
            puts(");\n");
        }

        எனில் (மாறிலி AstAssert* மாறிலி assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } இல்லையெனில் எனில் (மாறிலி AstAssertIntrinsic* மாறிலி assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    வெறுமை visit(AstAssocArrayDType* nodep) override {
        எனில் (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } இல்லையெனில் {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = பொய்;
            puts("[");
            iterateConst(nodep->சாவி_DTypep());
            puts("]");
            m_arrayPost = உண்மை;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    வெறுமை visit(AstBasicDType* nodep) override {
        எனில் (m_arrayPost) திரும்பு;
        putfs(nodep, nodep->prettyName());
        எனில் (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        எனில் (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) திரும்பு;
        emitPacked();
        எனில் (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } இல்லையெனில் எனில் (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->இடது()));
            puts(":");
            puts(cvtToStr(nodep->வலது()));
            puts("] ");
        }
    }
    வெறுமை visit(AstConstDType* nodep) override {
        எனில் (m_arrayPost) திரும்பு;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    வெறுமை visit(AstDynArrayDType* nodep) override {
        எனில் (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } இல்லையெனில் {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    வெறுமை visit(AstEnumDType* nodep) override {
        எனில் (m_arrayPost) திரும்பு;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        எனில் (Ast_முனை_Module* மாறிலி classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    வெறுமை visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        எனில் (nodep->nextp()) puts(",");
        puts("\n");
    }
    வெறுமை visit(Ast_முனை_Array_DType* nodep) override {
        எனில் (!m_arrayPost) {
            எனில் (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.தள்ளு_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } இல்லையெனில் {
            எனில் (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = பொய்;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = உண்மை;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    வெறுமை visit(AstIfaceRefDType* nodep) override {
        எனில் (m_arrayPost) {
            puts(" (");
            எனில் (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } இல்லையெனில் {
                puts("????");
            }
            puts(")");
            திரும்பு;
        }
        puts(nodep->ifaceName());
    }
    வெறுமை visit(AstRefDType* nodep) override {
        எனில் (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } இல்லையெனில் {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    வெறுமை visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    வெறுமை visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    வெறுமை visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        எனில் (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } இல்லையெனில் {
            puts("????");
        }
        puts(");\n");
    }
    வெறுமை visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        எனில் (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = உண்மை;
            iterateConst(nodep->varp());
        } இல்லையெனில் {
            puts(nodep->prettyName());
        }
        எனில் (nodep->nextp()) puts(", ");
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        எனில் (m_arrayPost) திரும்பு;
        puts(nodep->verilogKwd() + " ");
        எனில் (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            க்கு (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    வெறுமை visit(AstMemberDType* nodep) override {
        எனில் (m_arrayPost) திரும்பு;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    வெறுமை visit(AstQueueDType* nodep) override {
        எனில் (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } இல்லையெனில் {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = பொய்;
            puts("[$");
            எனில் (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = உண்மை;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        எனில் (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } இல்லையெனில் {
            putfs(nodep, nodep->prettyName());
        }
        எனில் (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    வெறுமை visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    வெறுமை visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    வெறுமை visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    வெறுமை visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    வெறுமை visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    வெறுமை visit(AstPropSpec* nodep) override {
        எனில் (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            க்கு (Ast_முனை* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                எனில் (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        எனில் (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        எனில் (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        எனில் (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    வெறுமை visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    வெறுமை visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    வெறுமை visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    வெறுமை visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        எனில் (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    வெறுமை visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = உண்மை;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } இல்லையெனில் {
            எனில் (nodep->varp()) {
                எனில் (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } இல்லையெனில் {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } இல்லையெனில் {
                putfs(nodep, nodep->name());
            }
        }
    }
    வெறுமை visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        எனில் (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } இல்லையெனில் {
            puts(nodep->prettyName());
        }
    }
    வெறுமை visit(AstConst* nodep) override { putfs(nodep, nodep->இலக்கம்().ascii(m_prefixed, உண்மை)); }
    வெறுமை visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    வெறுமை visit(Ast_உச்சி_Scope* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = பொய்;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = உண்மை;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    வெறுமை visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    வெறுமை visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        எனில் (nodep->isCycleDelay()) {
            puts("##");
        } இல்லையெனில் {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = பொய்;
        எனில் (Ast_முனை_கோவை* மாறிலி fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } இல்லையெனில் {
            iterateConst(nodep->lhsp());
        }
        எனில் (!m_suppressSemi) {
            puts(";\n");
        } இல்லையெனில் {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    வெறுமை visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        எனில் (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } இல்லையெனில் {
            puts(";\n");
        }
    }
    வெறுமை visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    வெறுமை visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    வெறுமை visit(AstVarScope*) override {}
    வெறுமை visit(AstTraceDecl*) override {}
    வெறுமை visit(AstTraceInc*) override {}
    // NOPs
    வெறுமை visit(AstPragma*) override {}
    வெறுமை visit(AstStmtPragma*) override {}
    வெறுமை visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    வெறுமை visit(Ast_முனை* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        எனில் (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

பொது:
    explicit EmitVBaseVisitorConst(தருக்க suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = இயல்பு;
};

//######################################################################
// Emit to an output file

வகுப்பு EmitVFileVisitor final : பொது EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    வெறுமை putsNoTracking(மாறிலி string& str) override { m_of.putsNoTracking(str); }
    வெறுமை puts(மாறிலி string& str) override { m_of.puts(str); }
    வெறுமை putbs(மாறிலி string& str) override { m_of.putbs(str); }
    வெறுமை putfs(Ast_முனை*, மாறிலி string& str) override { putbs(str); }
    வெறுமை putqs(Ast_முனை*, மாறிலி string& str) override { putbs(str); }

பொது:
    EmitVFileVisitor(Ast_முனை* nodep, V3OutVFile& of, தருக்க suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = இயல்பு;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

வகுப்பு EmitVStreamVisitor final : பொது EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    மாறிலி தருக்க m_tracking;  // Use line tracking
    // METHODS
    வெறுமை putsNoTracking(மாறிலி string& str) override { m_os.putsNoTracking(str); }
    வெறுமை puts(மாறிலி string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    வெறுமை putbs(மாறிலி string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    வெறுமை putfs(Ast_முனை*, மாறிலி string& str) override { putbs(str); }
    வெறுமை putqs(Ast_முனை*, மாறிலி string& str) override { putbs(str); }

பொது:
    EmitVStreamVisitor(மாறிலி Ast_முனை* nodep, std::ostream& os, தருக்க tracking, தருக்க suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_முனை*>(nodep));
    }
    ~EmitVStreamVisitor() override = இயல்பு;
};

//######################################################################
// EmitV class functions

வெறுமை V3EmitV::verilog_For_மரம்(மாறிலி Ast_முனை* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ பொய், பொய்}; }
}

வெறுமை V3EmitV::debug_Verilog_For_மரம்(மாறிலி Ast_முனை* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ உண்மை, உண்மை}; }
}

std::string V3EmitV::debug_Verilog_For_மரம்(மாறிலி Ast_முனை* nodep) {
    std::stringstream ss;
    debug_Verilog_For_மரம்(nodep, ss);
    திரும்பு ss.str();
}

வெறுமை V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    க்கு (Ast_முனை_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), முனை_File)) {
        AstVFile* மாறிலி vfilep = VN_CAST(filep, VFile);
        எனில் (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, பொய்};
        }
    }
}

வெறுமை V3EmitV::debugEmitV(மாறிலி string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, உண்மை};
}
