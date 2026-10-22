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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3EmitV.h"

#അന്തര്ഭാവ "V3EmitCBase.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

വർഗ്ഗം EmitVBaseVisitorConst VL_NOT_FINAL : പൊതു VNVisitorConst {
    // STATE - across all visitors
    സ്ഥിരം തർക്കം m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_വൃക്ഷം* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    തർക്കം m_suppressSemi = അസത്യം;  // Non-statement, don't print ;
    തർക്കം m_suppressVarSemi = അസത്യം;  // Suppress emitting semicolon for AstVars
    തർക്കം m_suppressSampled = അസത്യം;  // Suppress emitting sampled in assertion properties
    തർക്കം m_arrayPost = അസത്യം;  // Print array information that goes after identifier (vs after)
    തർക്കം m_prefixed = സത്യം;  // Whether constants need to be prefixed
    std::deque<Ast_കണ്ണി_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    മായാ ശൂന്യം puts(സ്ഥിരം string& str) = 0;
    മായാ ശൂന്യം putbs(സ്ഥിരം string& str) = 0;
    മായാ ശൂന്യം putfs(Ast_കണ്ണി* nodep, സ്ഥിരം string& str) = 0;  // Fileline and node %% mark
    മായാ ശൂന്യം putqs(Ast_കണ്ണി* nodep, സ്ഥിരം string& str) = 0;  // Fileline quiet w/o %% mark
    മായാ ശൂന്യം putsNoTracking(സ്ഥിരം string& str) = 0;
    മായാ ശൂന്യം putsQuoted(സ്ഥിരം string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    ശൂന്യം iterateAndCommaConstNull(Ast_കണ്ണി* nodep) {
        വേണ്ടി (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            എങ്കിൽ (nodep->nextp()) puts(", ");
        }
    }
    ശൂന്യം emitPacked() {
        വേണ്ടി (Ast_കണ്ണി_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    ശൂന്യം emitNodesWithText(Ast_കണ്ണി* nodesp, തർക്കം tracking, സ്ഥിരം std::string& separator) {
        വേണ്ടി (Ast_കണ്ണി* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstText* സ്ഥിരം textp = VN_CAST(nodep, Text)) {
                എങ്കിൽ (tracking) {
                    puts(textp->text());
                } അല്ലെങ്കിൽ {
                    putsNoTracking(textp->text());
                }
            } അല്ലെങ്കിൽ {
                iterateConst(nodep);
            }
            എങ്കിൽ (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    ശൂന്യം visit(AstPort* nodep) override {}
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    ശൂന്യം visit(AstGenBlock* nodep) override {
        സ്ഥിരം std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    ശൂന്യം visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ശൂന്യം visit(AstGenCaseItem* nodep) override {
        എങ്കിൽ (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } അല്ലെങ്കിൽ {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    ശൂന്യം visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = സത്യം;
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
    ശൂന്യം visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        എങ്കിൽ (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        എങ്കിൽ (nodep->name() == "") {
            putbs("begin\n");
        } അല്ലെങ്കിൽ {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    ശൂന്യം visit(AstFork* nodep) override {
        എങ്കിൽ (nodep->name() == "") {
            putbs("fork\n");
        } അല്ലെങ്കിൽ {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    ശൂന്യം visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (സ്ഥിരം AstAssignW* സ്ഥിരം ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            എങ്കിൽ (!ap->nextp()) {
                putfs(nodep, "assign ");
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                എങ്കിൽ (!m_suppressSemi) puts(";\n");
                തിരികെ;
            }
        }
        putfs(nodep, "always ");
        എങ്കിൽ (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        അല്ലെങ്കിൽ {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        എങ്കിൽ (!m_suppressSemi) puts(";\n");
    }
    ശൂന്യം visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        എങ്കിൽ (!m_suppressSemi) puts(";\n");
    }
    ശൂന്യം visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        എങ്കിൽ (!m_suppressSemi) puts(";\n");
    }
    ശൂന്യം visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        എങ്കിൽ (!m_suppressSemi) puts(";\n");
    }
    ശൂന്യം visit(AstBreak*) override {
        putbs("break");
        എങ്കിൽ (!m_suppressSemi) puts(";\n");
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        വേണ്ടി (Ast_കണ്ണി* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            എങ്കിൽ (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        എങ്കിൽ (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        എങ്കിൽ (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    ശൂന്യം visit(AstCase* nodep) override {
        putfs(nodep, "");
        എങ്കിൽ (nodep->priorityPragma()) puts("priority ");
        എങ്കിൽ (nodep->uniquePragma()) puts("unique ");
        എങ്കിൽ (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        എങ്കിൽ (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            എങ്കിൽ (nodep->fullPragma()) puts(" full_case");
            എങ്കിൽ (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        എങ്കിൽ (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } അല്ലെങ്കിൽ {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstContinue*) override {
        putbs("continue");
        എങ്കിൽ (!m_suppressSemi) puts(";\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Decl*) override {}  // N/A
    ശൂന്യം visit(AstCoverInc*) override {}  // N/A
    ശൂന്യം visit(AstCoverToggle*) override {}  // N/A

    ശൂന്യം visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        എങ്കിൽ (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    ശൂന്യം visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        എങ്കിൽ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    ശൂന്യം emitCoverageOption(Ast_കണ്ണി* nodep, തർക്കം typeOption, VCoverOptionType optType,
                            Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    ശൂന്യം visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ശൂന്യം visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ശൂന്യം visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        എങ്കിൽ (nodep->isArray()) {
            puts("[");
            എങ്കിൽ (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        എങ്കിൽ (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->transp()) {
            puts(" = ");
            വേണ്ടി (Ast_കണ്ണി* setp = nodep->transp(); setp; setp = setp->nextp()) {
                എങ്കിൽ (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            വേണ്ടി (Ast_കണ്ണി* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                എങ്കിൽ (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        എങ്കിൽ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ശൂന്യം visit(AstCoverWith* nodep) override {
        സ്ഥിരം തർക്കം rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        എങ്കിൽ (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        എങ്കിൽ (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    ശൂന്യം visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        എങ്കിൽ (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        എങ്കിൽ (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    ശൂന്യം visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        എങ്കിൽ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ശൂന്യം visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    ശൂന്യം visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    ശൂന്യം visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        എങ്കിൽ (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        എങ്കിൽ (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    ശൂന്യം visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    ശൂന്യം visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        എങ്കിൽ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        എങ്കിൽ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } അല്ലെങ്കിൽ {
            puts(";\n");
        }
    }
    ശൂന്യം visit(AstCoverTransSet* nodep) override {
        puts("(");
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    ശൂന്യം visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    ശൂന്യം visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        എങ്കിൽ (AstConst* സ്ഥിരം lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->അക്കം().toString());
        } അല്ലെങ്കിൽ {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    ശൂന്യം visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    ശൂന്യം visit(Ast_മൂല്യം_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    ശൂന്യം visit_കണ്ണി_Display(Ast_കണ്ണി* nodep, Ast_കണ്ണി* fileOrStrgp, സ്ഥിരം string& text,
                          Ast_കണ്ണി* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        എങ്കിൽ (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        വേണ്ടി (Ast_കണ്ണി* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    ശൂന്യം visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    ശൂന്യം visit(AstDisplay* nodep) override {
        visit_കണ്ണി_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ശൂന്യം visit(AstElabDisplay* nodep) override {
        visit_കണ്ണി_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ശൂന്യം visit(AstFScanF* nodep) override {
        visit_കണ്ണി_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        visit_കണ്ണി_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    ശൂന്യം visit(AstSFormat* nodep) override {
        visit_കണ്ണി_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ശൂന്യം visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    ശൂന്യം visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        visit_കണ്ണി_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    ശൂന്യം visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    ശൂന്യം visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    ശൂന്യം visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        എങ്കിൽ (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    ശൂന്യം visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        എങ്കിൽ (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    ശൂന്യം visit(AstJumpBlock* nodep) override {
        // Allocate label number
        സ്ഥിരം size_t n = m_labelNumbers.size();
        സ്ഥിരം തർക്കം newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    ശൂന്യം visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        സ്ഥിരം auto it = m_labelNumbers.find(nodep->blockp());
        സ്ഥിരം std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        എങ്കിൽ (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        എങ്കിൽ (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        എങ്കിൽ (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        എങ്കിൽ (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    ശൂന്യം visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    ശൂന്യം visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            തിരികെ;
        }
        // Special case when the AstLoopTest is last for output readability
        എങ്കിൽ (Ast_കണ്ണി* lastp = nodep->stmtsp()) {
            ആകുമ്പോൾ (Ast_കണ്ണി* സ്ഥിരം nextp = lastp->nextp()) lastp = nextp;
            എങ്കിൽ (AstLoopTest* സ്ഥിരം testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                വേണ്ടി (Ast_കണ്ണി* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                തിരികെ;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        putfs(nodep, "");
        എങ്കിൽ (സ്ഥിരം AstIf* സ്ഥിരം ifp = VN_CAST(nodep, If)) {
            എങ്കിൽ (ifp->priorityPragma()) puts("priority ");
            എങ്കിൽ (ifp->uniquePragma()) puts("unique ");
            എങ്കിൽ (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        എങ്കിൽ (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ശൂന്യം visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        എങ്കിൽ (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            എങ്കിൽ (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    ശൂന്യം visit(AstSampled* nodep) override {
        എങ്കിൽ (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    ശൂന്യം visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ശൂന്യം visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        എങ്കിൽ (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ശൂന്യം visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        എങ്കിൽ (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ശൂന്യം visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ശൂന്യം visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ശൂന്യം visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        എങ്കിൽ (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ശൂന്യം visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ശൂന്യം visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    ശൂന്യം visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    ശൂന്യം visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    ശൂന്യം visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    ശൂന്യം visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ശൂന്യം visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), സത്യം, nodep->separator());
        puts(nodep->suffix());
    }
    ശൂന്യം visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), സത്യം, "");
        puts(");\n");
    }
    ശൂന്യം visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), സത്യം, "");
        puts(")");
    }
    ശൂന്യം visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), അസത്യം, "");
        puts(");\n");
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), അസത്യം, "");
        puts(")");
    }

    ശൂന്യം visit(AstScopeName* nodep) override {}
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    ശൂന്യം visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    ശൂന്യം visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    മായാ ശൂന്യം emitVerilogFormat(Ast_കണ്ണി* nodep, സ്ഥിരം string& format, Ast_കണ്ണി* lhsp = nullptr,
                                   Ast_കണ്ണി* സ്ഥിരം rhsp = nullptr, Ast_കണ്ണി* thsp = nullptr,
                                   Ast_കണ്ണി* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        തർക്കം inPct = അസത്യം;
        putbs("");
        വേണ്ടി (സ്ഥിരം അക്ഷരം c : format) {
            എങ്കിൽ (!inPct && c == '%') {
                inPct = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } അല്ലെങ്കിൽ {  // Format character
                inPct = അസത്യം;
                തിരഞ്ഞെടുപ്പ് (c) {
                അവസ്ഥ '%': puts("%"); നിർത്തുക;
                അവസ്ഥ 'f': putfs(nodep, ""); നിർത്തുക;
                അവസ്ഥ 'k': putbs(""); നിർത്തുക;
                അവസ്ഥ 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    നിർത്തുക;
                }
                അവസ്ഥ 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    നിർത്തുക;
                }
                അവസ്ഥ 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    നിർത്തുക;
                }
                അവസ്ഥ 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    നിർത്തുക;
                }
                അവസ്ഥ 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    നിർത്തുക;
                }
                സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); നിർത്തുക;
                }
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ശൂന്യം visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        എങ്കിൽ (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    ശൂന്യം visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        വേണ്ടി (സ്ഥിരം AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            എങ്കിൽ (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    ശൂന്യം visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    ശൂന്യം visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    ശൂന്യം visit(AstRange* nodep) override {
        puts("[");
        എങ്കിൽ (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->ഇടത്_Const()));
            puts(":");
            puts(cvtToStr(nodep->വലത്_Const()));
        } അല്ലെങ്കിൽ {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    ശൂന്യം visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    ശൂന്യം visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        പൂർണ്ണസംഖ്യ offset = 0;
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->fromp()->dtypep();
        എങ്കിൽ (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* സ്ഥിരം basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        എങ്കിൽ (VN_IS(nodep->lsbp(), Const)) {
            എങ്കിൽ (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } അല്ലെങ്കിൽ {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } അല്ലെങ്കിൽ {
            iterateAndNextConstNull(nodep->lsbp());
            എങ്കിൽ (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    ശൂന്യം visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    ശൂന്യം visit(AstThisRef* /*nodep*/) override { puts("this"); }
    ശൂന്യം visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        എങ്കിൽ (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        എങ്കിൽ (AstSampled* സ്ഥിരം sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } അല്ലെങ്കിൽ {
            iterateAndNextConstNull(nodep->propp());
        }
        എങ്കിൽ (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } അല്ലെങ്കിൽ {
            puts(");\n");
        }

        എങ്കിൽ (സ്ഥിരം AstAssert* സ്ഥിരം assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstAssertIntrinsic* സ്ഥിരം assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    ശൂന്യം visit(AstAssocArrayDType* nodep) override {
        എങ്കിൽ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = അസത്യം;
            puts("[");
            iterateConst(nodep->താക്കോൽ_DTypep());
            puts("]");
            m_arrayPost = സത്യം;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ശൂന്യം visit(AstBasicDType* nodep) override {
        എങ്കിൽ (m_arrayPost) തിരികെ;
        putfs(nodep, nodep->prettyName());
        എങ്കിൽ (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        എങ്കിൽ (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) തിരികെ;
        emitPacked();
        എങ്കിൽ (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->ഇടത്()));
            puts(":");
            puts(cvtToStr(nodep->വലത്()));
            puts("] ");
        }
    }
    ശൂന്യം visit(AstConstDType* nodep) override {
        എങ്കിൽ (m_arrayPost) തിരികെ;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    ശൂന്യം visit(AstDynArrayDType* nodep) override {
        എങ്കിൽ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } അല്ലെങ്കിൽ {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ശൂന്യം visit(AstEnumDType* nodep) override {
        എങ്കിൽ (m_arrayPost) തിരികെ;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    ശൂന്യം visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        എങ്കിൽ (nodep->nextp()) puts(",");
        puts("\n");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Array_DType* nodep) override {
        എങ്കിൽ (!m_arrayPost) {
            എങ്കിൽ (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.തള്ളുക_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = അസത്യം;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = സത്യം;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        എങ്കിൽ (m_arrayPost) {
            puts(" (");
            എങ്കിൽ (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } അല്ലെങ്കിൽ {
                puts("????");
            }
            puts(")");
            തിരികെ;
        }
        puts(nodep->ifaceName());
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        എങ്കിൽ (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } അല്ലെങ്കിൽ {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    ശൂന്യം visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    ശൂന്യം visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        എങ്കിൽ (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } അല്ലെങ്കിൽ {
            puts("????");
        }
        puts(");\n");
    }
    ശൂന്യം visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        എങ്കിൽ (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = സത്യം;
            iterateConst(nodep->varp());
        } അല്ലെങ്കിൽ {
            puts(nodep->prettyName());
        }
        എങ്കിൽ (nodep->nextp()) puts(", ");
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        എങ്കിൽ (m_arrayPost) തിരികെ;
        puts(nodep->verilogKwd() + " ");
        എങ്കിൽ (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            വേണ്ടി (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    ശൂന്യം visit(AstMemberDType* nodep) override {
        എങ്കിൽ (m_arrayPost) തിരികെ;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    ശൂന്യം visit(AstQueueDType* nodep) override {
        എങ്കിൽ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = അസത്യം;
            puts("[$");
            എങ്കിൽ (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = സത്യം;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } അല്ലെങ്കിൽ {
            putfs(nodep, nodep->prettyName());
        }
        എങ്കിൽ (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    ശൂന്യം visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    ശൂന്യം visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    ശൂന്യം visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    ശൂന്യം visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    ശൂന്യം visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    ശൂന്യം visit(AstPropSpec* nodep) override {
        എങ്കിൽ (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            വേണ്ടി (Ast_കണ്ണി* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                എങ്കിൽ (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        എങ്കിൽ (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        എങ്കിൽ (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        എങ്കിൽ (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    ശൂന്യം visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    ശൂന്യം visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    ശൂന്യം visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    ശൂന്യം visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        എങ്കിൽ (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    ശൂന്യം visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = സത്യം;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->varp()) {
                എങ്കിൽ (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } അല്ലെങ്കിൽ {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } അല്ലെങ്കിൽ {
                putfs(nodep, nodep->name());
            }
        }
    }
    ശൂന്യം visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        എങ്കിൽ (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } അല്ലെങ്കിൽ {
            puts(nodep->prettyName());
        }
    }
    ശൂന്യം visit(AstConst* nodep) override { putfs(nodep, nodep->അക്കം().ascii(m_prefixed, സത്യം)); }
    ശൂന്യം visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    ശൂന്യം visit(Ast_മുകൾ_Scope* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = അസത്യം;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = സത്യം;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    ശൂന്യം visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ശൂന്യം visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        എങ്കിൽ (nodep->isCycleDelay()) {
            puts("##");
        } അല്ലെങ്കിൽ {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = അസത്യം;
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } അല്ലെങ്കിൽ {
            iterateConst(nodep->lhsp());
        }
        എങ്കിൽ (!m_suppressSemi) {
            puts(";\n");
        } അല്ലെങ്കിൽ {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ശൂന്യം visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        എങ്കിൽ (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } അല്ലെങ്കിൽ {
            puts(";\n");
        }
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    ശൂന്യം visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    ശൂന്യം visit(AstVarScope*) override {}
    ശൂന്യം visit(AstTraceDecl*) override {}
    ശൂന്യം visit(AstTraceInc*) override {}
    // NOPs
    ശൂന്യം visit(AstPragma*) override {}
    ശൂന്യം visit(AstStmtPragma*) override {}
    ശൂന്യം visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        എങ്കിൽ (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

പൊതു:
    explicit EmitVBaseVisitorConst(തർക്കം suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Emit to an output file

വർഗ്ഗം EmitVFileVisitor final : പൊതു EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    ശൂന്യം putsNoTracking(സ്ഥിരം string& str) override { m_of.putsNoTracking(str); }
    ശൂന്യം puts(സ്ഥിരം string& str) override { m_of.puts(str); }
    ശൂന്യം putbs(സ്ഥിരം string& str) override { m_of.putbs(str); }
    ശൂന്യം putfs(Ast_കണ്ണി*, സ്ഥിരം string& str) override { putbs(str); }
    ശൂന്യം putqs(Ast_കണ്ണി*, സ്ഥിരം string& str) override { putbs(str); }

പൊതു:
    EmitVFileVisitor(Ast_കണ്ണി* nodep, V3OutVFile& of, തർക്കം suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

വർഗ്ഗം EmitVStreamVisitor final : പൊതു EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    സ്ഥിരം തർക്കം m_tracking;  // Use line tracking
    // METHODS
    ശൂന്യം putsNoTracking(സ്ഥിരം string& str) override { m_os.putsNoTracking(str); }
    ശൂന്യം puts(സ്ഥിരം string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    ശൂന്യം putbs(സ്ഥിരം string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    ശൂന്യം putfs(Ast_കണ്ണി*, സ്ഥിരം string& str) override { putbs(str); }
    ശൂന്യം putqs(Ast_കണ്ണി*, സ്ഥിരം string& str) override { putbs(str); }

പൊതു:
    EmitVStreamVisitor(സ്ഥിരം Ast_കണ്ണി* nodep, std::ostream& os, തർക്കം tracking, തർക്കം suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_കണ്ണി*>(nodep));
    }
    ~EmitVStreamVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// EmitV class functions

ശൂന്യം V3EmitV::verilog_For_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ അസത്യം, അസത്യം}; }
}

ശൂന്യം V3EmitV::debug_Verilog_For_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ സത്യം, സത്യം}; }
}

std::string V3EmitV::debug_Verilog_For_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* nodep) {
    std::stringstream ss;
    debug_Verilog_For_വൃക്ഷം(nodep, ss);
    തിരികെ ss.str();
}

ശൂന്യം V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    വേണ്ടി (Ast_കണ്ണി_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), കണ്ണി_File)) {
        AstVFile* സ്ഥിരം vfilep = VN_CAST(filep, VFile);
        എങ്കിൽ (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, അസത്യം};
        }
    }
}

ശൂന്യം V3EmitV::debugEmitV(സ്ഥിരം string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, സത്യം};
}
