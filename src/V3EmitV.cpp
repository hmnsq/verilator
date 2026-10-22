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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

ଶ୍ରେଣୀ EmitVBaseVisitorConst VL_NOT_FINAL : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE - across all visitors
    ଅଚଳ ତାର୍କିକ m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_ବୃକ୍ଷ* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    ତାର୍କିକ m_suppressSemi = ମିଥ୍ୟା;  // Non-statement, don't print ;
    ତାର୍କିକ m_suppressVarSemi = ମିଥ୍ୟା;  // Suppress emitting semicolon for AstVars
    ତାର୍କିକ m_suppressSampled = ମିଥ୍ୟା;  // Suppress emitting sampled in assertion properties
    ତାର୍କିକ m_arrayPost = ମିଥ୍ୟା;  // Print array information that goes after identifier (vs after)
    ତାର୍କିକ m_prefixed = ସତ୍ୟ;  // Whether constants need to be prefixed
    std::deque<Ast_ଗଣ୍ଠି_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    ଆଭାସୀ ଶୂନ୍ୟ puts(ଅଚଳ string& str) = 0;
    ଆଭାସୀ ଶୂନ୍ୟ putbs(ଅଚଳ string& str) = 0;
    ଆଭାସୀ ଶୂନ୍ୟ putfs(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& str) = 0;  // Fileline and node %% mark
    ଆଭାସୀ ଶୂନ୍ୟ putqs(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& str) = 0;  // Fileline quiet w/o %% mark
    ଆଭାସୀ ଶୂନ୍ୟ putsNoTracking(ଅଚଳ string& str) = 0;
    ଆଭାସୀ ଶୂନ୍ୟ putsQuoted(ଅଚଳ string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    ଶୂନ୍ୟ iterateAndCommaConstNull(Ast_ଗଣ୍ଠି* nodep) {
        ପାଇଁ (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            ଯଦି (nodep->nextp()) puts(", ");
        }
    }
    ଶୂନ୍ୟ emitPacked() {
        ପାଇଁ (Ast_ଗଣ୍ଠି_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    ଶୂନ୍ୟ emitNodesWithText(Ast_ଗଣ୍ଠି* nodesp, ତାର୍କିକ tracking, ଅଚଳ std::string& separator) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ଯଦି (ଅଚଳ AstText* ଅଚଳ textp = VN_CAST(nodep, Text)) {
                ଯଦି (tracking) {
                    puts(textp->text());
                } ଅନ୍ୟଥା {
                    putsNoTracking(textp->text());
                }
            } ଅନ୍ୟଥା {
                iterateConst(nodep);
            }
            ଯଦି (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    ଶୂନ୍ୟ visit(AstPort* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        ଅଚଳ std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    ଶୂନ୍ୟ visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ଶୂନ୍ୟ visit(AstGenCaseItem* nodep) override {
        ଯଦି (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } ଅନ୍ୟଥା {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    ଶୂନ୍ୟ visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ସତ୍ୟ;
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
    ଶୂନ୍ୟ visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        ଯଦି (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        ଯଦି (nodep->name() == "") {
            putbs("begin\n");
        } ଅନ୍ୟଥା {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        ଯଦି (nodep->name() == "") {
            putbs("fork\n");
        } ଅନ୍ୟଥା {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    ଶୂନ୍ୟ visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (ଅଚଳ AstAssignW* ଅଚଳ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            ଯଦି (!ap->nextp()) {
                putfs(nodep, "assign ");
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                ଯଦି (!m_suppressSemi) puts(";\n");
                ଫେରନ୍ତୁ;
            }
        }
        putfs(nodep, "always ");
        ଯଦି (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        ଅନ୍ୟଥା {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        ଯଦି (!m_suppressSemi) puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        ଯଦି (!m_suppressSemi) puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        ଯଦି (!m_suppressSemi) puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        ଯଦି (!m_suppressSemi) puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstBreak*) override {
        putbs("break");
        ଯଦି (!m_suppressSemi) puts(";\n");
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        ପାଇଁ (Ast_ଗଣ୍ଠି* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            ଯଦି (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        ଯଦି (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        ଯଦି (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    ଶୂନ୍ୟ visit(AstCase* nodep) override {
        putfs(nodep, "");
        ଯଦି (nodep->priorityPragma()) puts("priority ");
        ଯଦି (nodep->uniquePragma()) puts("unique ");
        ଯଦି (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        ଯଦି (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            ଯଦି (nodep->fullPragma()) puts(" full_case");
            ଯଦି (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        ଯଦି (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } ଅନ୍ୟଥା {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstContinue*) override {
        putbs("continue");
        ଯଦି (!m_suppressSemi) puts(";\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Decl*) override {}  // N/A
    ଶୂନ୍ୟ visit(AstCoverInc*) override {}  // N/A
    ଶୂନ୍ୟ visit(AstCoverToggle*) override {}  // N/A

    ଶୂନ୍ୟ visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        ଯଦି (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    ଶୂନ୍ୟ visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        ଯଦି (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    ଶୂନ୍ୟ emitCoverageOption(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ typeOption, VCoverOptionType optType,
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ଶୂନ୍ୟ visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ଶୂନ୍ୟ visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        ଯଦି (nodep->isArray()) {
            puts("[");
            ଯଦି (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        ଯଦି (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } ଅନ୍ୟଥା ଯଦି (nodep->transp()) {
            puts(" = ");
            ପାଇଁ (Ast_ଗଣ୍ଠି* setp = nodep->transp(); setp; setp = setp->nextp()) {
                ଯଦି (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } ଅନ୍ୟଥା ଯଦି (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            ପାଇଁ (Ast_ଗଣ୍ଠି* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                ଯଦି (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        ଯଦି (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstCoverWith* nodep) override {
        ଅଚଳ ତାର୍କିକ rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        ଯଦି (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        ଯଦି (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        ଯଦି (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        ଯଦି (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    ଶୂନ୍ୟ visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        ଯଦି (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    ଶୂନ୍ୟ visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        ଯଦି (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        ଯଦି (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    ଶୂନ୍ୟ visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    ଶୂନ୍ୟ visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        ଯଦି (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        ଯଦି (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } ଅନ୍ୟଥା {
            puts(";\n");
        }
    }
    ଶୂନ୍ୟ visit(AstCoverTransSet* nodep) override {
        puts("(");
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    ଶୂନ୍ୟ visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        ଯଦି (AstConst* ଅଚଳ lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->ଅଙ୍କ().toString());
        } ଅନ୍ୟଥା {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    ଶୂନ୍ୟ visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    ଶୂନ୍ୟ visit(Ast_ମୂଲ୍ୟ_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    ଶୂନ୍ୟ visit_ଗଣ୍ଠି_Display(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* fileOrStrgp, ଅଚଳ string& text,
                          Ast_ଗଣ୍ଠି* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ଯଦି (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        ପାଇଁ (Ast_ଗଣ୍ଠି* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        visit_ଗଣ୍ଠି_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ଶୂନ୍ୟ visit(AstElabDisplay* nodep) override {
        visit_ଗଣ୍ଠି_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ଶୂନ୍ୟ visit(AstFScanF* nodep) override {
        visit_ଗଣ୍ଠି_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        visit_ଗଣ୍ଠି_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    ଶୂନ୍ୟ visit(AstSFormat* nodep) override {
        visit_ଗଣ୍ଠି_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ଶୂନ୍ୟ visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    ଶୂନ୍ୟ visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        visit_ଗଣ୍ଠି_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    ଶୂନ୍ୟ visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ଯଦି (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ଯଦି (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ଅଚଳ size_t n = m_labelNumbers.size();
        ଅଚଳ ତାର୍କିକ newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ଅଚଳ auto it = m_labelNumbers.find(nodep->blockp());
        ଅଚଳ std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ଯଦି (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        ଯଦି (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        ଯଦି (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        ଯଦି (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            ଫେରନ୍ତୁ;
        }
        // Special case when the AstLoopTest is last for output readability
        ଯଦି (Ast_ଗଣ୍ଠି* lastp = nodep->stmtsp()) {
            ଯେତେବେଳେ (Ast_ଗଣ୍ଠି* ଅଚଳ nextp = lastp->nextp()) lastp = nextp;
            ଯଦି (AstLoopTest* ଅଚଳ testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                ପାଇଁ (Ast_ଗଣ୍ଠି* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                ଫେରନ୍ତୁ;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        putfs(nodep, "");
        ଯଦି (ଅଚଳ AstIf* ଅଚଳ ifp = VN_CAST(nodep, If)) {
            ଯଦି (ifp->priorityPragma()) puts("priority ");
            ଯଦି (ifp->uniquePragma()) puts("unique ");
            ଯଦି (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        ଯଦି (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ଶୂନ୍ୟ visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        ଯଦି (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            ଯଦି (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstSampled* nodep) override {
        ଯଦି (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    ଶୂନ୍ୟ visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        ଯଦି (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        ଯଦି (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        ଯଦି (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    ଶୂନ୍ୟ visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    ଶୂନ୍ୟ visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ଶୂନ୍ୟ visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), ସତ୍ୟ, nodep->separator());
        puts(nodep->suffix());
    }
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), ସତ୍ୟ, "");
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), ସତ୍ୟ, "");
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), ମିଥ୍ୟା, "");
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), ମିଥ୍ୟା, "");
        puts(")");
    }

    ଶୂନ୍ୟ visit(AstScopeName* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    ଶୂନ୍ୟ visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    ଆଭାସୀ ଶୂନ୍ୟ emitVerilogFormat(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& format, Ast_ଗଣ୍ଠି* lhsp = nullptr,
                                   Ast_ଗଣ୍ଠି* ଅଚଳ rhsp = nullptr, Ast_ଗଣ୍ଠି* thsp = nullptr,
                                   Ast_ଗଣ୍ଠି* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        ତାର୍କିକ inPct = ମିଥ୍ୟା;
        putbs("");
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : format) {
            ଯଦି (!inPct && c == '%') {
                inPct = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } ଅନ୍ୟଥା {  // Format character
                inPct = ମିଥ୍ୟା;
                ବାଛ (c) {
                ସ୍ଥିତି '%': puts("%"); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'f': putfs(nodep, ""); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'k': putbs(""); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    ଭାଙ୍ଗ;
                }
                ସ୍ଥିତି 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    ଭାଙ୍ଗ;
                }
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); ଭାଙ୍ଗ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        ଯଦି (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        ପାଇଁ (ଅଚଳ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            ଯଦି (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    ଶୂନ୍ୟ visit(AstRange* nodep) override {
        puts("[");
        ଯଦି (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->ବାମ_Const()));
            puts(":");
            puts(cvtToStr(nodep->ଡାହାଣ_Const()));
        } ଅନ୍ୟଥା {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    ଶୂନ୍ୟ visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା offset = 0;
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->fromp()->dtypep();
        ଯଦି (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ଅଚଳ basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        ଯଦି (VN_IS(nodep->lsbp(), Const)) {
            ଯଦି (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } ଅନ୍ୟଥା {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } ଅନ୍ୟଥା {
            iterateAndNextConstNull(nodep->lsbp());
            ଯଦି (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    ଶୂନ୍ୟ visit(AstThisRef* /*nodep*/) override { puts("this"); }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        ଯଦି (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } ଅନ୍ୟଥା ଯଦି (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } ଅନ୍ୟଥା ଯଦି (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        ଯଦି (AstSampled* ଅଚଳ sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } ଅନ୍ୟଥା {
            iterateAndNextConstNull(nodep->propp());
        }
        ଯଦି (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } ଅନ୍ୟଥା {
            puts(");\n");
        }

        ଯଦି (ଅଚଳ AstAssert* ଅଚଳ assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstAssertIntrinsic* ଅଚଳ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    ଶୂନ୍ୟ visit(AstAssocArrayDType* nodep) override {
        ଯଦି (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ମିଥ୍ୟା;
            puts("[");
            iterateConst(nodep->ଚାବି_DTypep());
            puts("]");
            m_arrayPost = ସତ୍ୟ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ଶୂନ୍ୟ visit(AstBasicDType* nodep) override {
        ଯଦି (m_arrayPost) ଫେରନ୍ତୁ;
        putfs(nodep, nodep->prettyName());
        ଯଦି (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        ଯଦି (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) ଫେରନ୍ତୁ;
        emitPacked();
        ଯଦି (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } ଅନ୍ୟଥା ଯଦି (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->ବାମ()));
            puts(":");
            puts(cvtToStr(nodep->ଡାହାଣ()));
            puts("] ");
        }
    }
    ଶୂନ୍ୟ visit(AstConstDType* nodep) override {
        ଯଦି (m_arrayPost) ଫେରନ୍ତୁ;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    ଶୂନ୍ୟ visit(AstDynArrayDType* nodep) override {
        ଯଦି (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ଅନ୍ୟଥା {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ଶୂନ୍ୟ visit(AstEnumDType* nodep) override {
        ଯଦି (m_arrayPost) ଫେରନ୍ତୁ;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    ଶୂନ୍ୟ visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        ଯଦି (nodep->nextp()) puts(",");
        puts("\n");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Array_DType* nodep) override {
        ଯଦି (!m_arrayPost) {
            ଯଦି (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ଠେଲ_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } ଅନ୍ୟଥା {
            ଯଦି (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = ମିଥ୍ୟା;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = ସତ୍ୟ;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        ଯଦି (m_arrayPost) {
            puts(" (");
            ଯଦି (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } ଅନ୍ୟଥା {
                puts("????");
            }
            puts(")");
            ଫେରନ୍ତୁ;
        }
        puts(nodep->ifaceName());
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        ଯଦି (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } ଅନ୍ୟଥା {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    ଶୂନ୍ୟ visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    ଶୂନ୍ୟ visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        ଯଦି (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } ଅନ୍ୟଥା {
            puts("????");
        }
        puts(");\n");
    }
    ଶୂନ୍ୟ visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        ଯଦି (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = ସତ୍ୟ;
            iterateConst(nodep->varp());
        } ଅନ୍ୟଥା {
            puts(nodep->prettyName());
        }
        ଯଦି (nodep->nextp()) puts(", ");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        ଯଦି (m_arrayPost) ଫେରନ୍ତୁ;
        puts(nodep->verilogKwd() + " ");
        ଯଦି (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            ପାଇଁ (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    ଶୂନ୍ୟ visit(AstMemberDType* nodep) override {
        ଯଦି (m_arrayPost) ଫେରନ୍ତୁ;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstQueueDType* nodep) override {
        ଯଦି (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ମିଥ୍ୟା;
            puts("[$");
            ଯଦି (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = ସତ୍ୟ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } ଅନ୍ୟଥା {
            putfs(nodep, nodep->prettyName());
        }
        ଯଦି (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    ଶୂନ୍ୟ visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    ଶୂନ୍ୟ visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    ଶୂନ୍ୟ visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    ଶୂନ୍ୟ visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    ଶୂନ୍ୟ visit(AstPropSpec* nodep) override {
        ଯଦି (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            ପାଇଁ (Ast_ଗଣ୍ଠି* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                ଯଦି (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        ଯଦି (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        ଯଦି (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        ଯଦି (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    ଶୂନ୍ୟ visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    ଶୂନ୍ୟ visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    ଶୂନ୍ୟ visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    ଶୂନ୍ୟ visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        ଯଦି (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    ଶୂନ୍ୟ visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ସତ୍ୟ;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->varp()) {
                ଯଦି (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } ଅନ୍ୟଥା {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } ଅନ୍ୟଥା {
                putfs(nodep, nodep->name());
            }
        }
    }
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        ଯଦି (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } ଅନ୍ୟଥା {
            puts(nodep->prettyName());
        }
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override { putfs(nodep, nodep->ଅଙ୍କ().ascii(m_prefixed, ସତ୍ୟ)); }
    ଶୂନ୍ୟ visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    ଶୂନ୍ୟ visit(Ast_ଶୀର୍ଷ_Scope* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = ମିଥ୍ୟା;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = ସତ୍ୟ;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        ଯଦି (nodep->isCycleDelay()) {
            puts("##");
        } ଅନ୍ୟଥା {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = ମିଥ୍ୟା;
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } ଅନ୍ୟଥା {
            iterateConst(nodep->lhsp());
        }
        ଯଦି (!m_suppressSemi) {
            puts(";\n");
        } ଅନ୍ୟଥା {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        ଯଦି (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } ଅନ୍ୟଥା {
            puts(";\n");
        }
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    ଶୂନ୍ୟ visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    ଶୂନ୍ୟ visit(AstVarScope*) override {}
    ଶୂନ୍ୟ visit(AstTraceDecl*) override {}
    ଶୂନ୍ୟ visit(AstTraceInc*) override {}
    // NOPs
    ଶୂନ୍ୟ visit(AstPragma*) override {}
    ଶୂନ୍ୟ visit(AstStmtPragma*) override {}
    ଶୂନ୍ୟ visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        ଯଦି (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

ସର୍ବସାଧାରଣ:
    explicit EmitVBaseVisitorConst(ତାର୍କିକ suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Emit to an output file

ଶ୍ରେଣୀ EmitVFileVisitor final : ସର୍ବସାଧାରଣ EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    ଶୂନ୍ୟ putsNoTracking(ଅଚଳ string& str) override { m_of.putsNoTracking(str); }
    ଶୂନ୍ୟ puts(ଅଚଳ string& str) override { m_of.puts(str); }
    ଶୂନ୍ୟ putbs(ଅଚଳ string& str) override { m_of.putbs(str); }
    ଶୂନ୍ୟ putfs(Ast_ଗଣ୍ଠି*, ଅଚଳ string& str) override { putbs(str); }
    ଶୂନ୍ୟ putqs(Ast_ଗଣ୍ଠି*, ଅଚଳ string& str) override { putbs(str); }

ସର୍ବସାଧାରଣ:
    EmitVFileVisitor(Ast_ଗଣ୍ଠି* nodep, V3OutVFile& of, ତାର୍କିକ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

ଶ୍ରେଣୀ EmitVStreamVisitor final : ସର୍ବସାଧାରଣ EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    ଅଚଳ ତାର୍କିକ m_tracking;  // Use line tracking
    // METHODS
    ଶୂନ୍ୟ putsNoTracking(ଅଚଳ string& str) override { m_os.putsNoTracking(str); }
    ଶୂନ୍ୟ puts(ଅଚଳ string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    ଶୂନ୍ୟ putbs(ଅଚଳ string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    ଶୂନ୍ୟ putfs(Ast_ଗଣ୍ଠି*, ଅଚଳ string& str) override { putbs(str); }
    ଶୂନ୍ୟ putqs(Ast_ଗଣ୍ଠି*, ଅଚଳ string& str) override { putbs(str); }

ସର୍ବସାଧାରଣ:
    EmitVStreamVisitor(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, std::ostream& os, ତାର୍କିକ tracking, ତାର୍କିକ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ଗଣ୍ଠି*>(nodep));
    }
    ~EmitVStreamVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// EmitV class functions

ଶୂନ୍ୟ V3EmitV::verilog_For_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ମିଥ୍ୟା, ମିଥ୍ୟା}; }
}

ଶୂନ୍ୟ V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ସତ୍ୟ, ସତ୍ୟ}; }
}

std::string V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    std::stringstream ss;
    debug_Verilog_For_ବୃକ୍ଷ(nodep, ss);
    ଫେରନ୍ତୁ ss.str();
}

ଶୂନ୍ୟ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ପାଇଁ (Ast_ଗଣ୍ଠି_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ଗଣ୍ଠି_File)) {
        AstVFile* ଅଚଳ vfilep = VN_CAST(filep, VFile);
        ଯଦି (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, ମିଥ୍ୟା};
        }
    }
}

ଶୂନ୍ୟ V3EmitV::debugEmitV(ଅଚଳ string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, ସତ୍ୟ};
}
