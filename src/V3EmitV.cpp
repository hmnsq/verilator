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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3EmitV.h"

#અન્તર્ભાવ "V3EmitCBase.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

વર્ગ EmitVBaseVisitorConst VL_NOT_FINAL : જાહેર VNVisitorConst {
    // STATE - across all visitors
    અચલ તાર્કિક m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_વૃક્ષ* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    તાર્કિક m_suppressSemi = ખોટું;  // Non-statement, don't print ;
    તાર્કિક m_suppressVarSemi = ખોટું;  // Suppress emitting semicolon for AstVars
    તાર્કિક m_suppressSampled = ખોટું;  // Suppress emitting sampled in assertion properties
    તાર્કિક m_arrayPost = ખોટું;  // Print array information that goes after identifier (vs after)
    તાર્કિક m_prefixed = સાચું;  // Whether constants need to be prefixed
    std::deque<Ast_ગાંઠ_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    આભાસી શૂન્ય puts(અચલ string& str) = 0;
    આભાસી શૂન્ય putbs(અચલ string& str) = 0;
    આભાસી શૂન્ય putfs(Ast_ગાંઠ* nodep, અચલ string& str) = 0;  // Fileline and node %% mark
    આભાસી શૂન્ય putqs(Ast_ગાંઠ* nodep, અચલ string& str) = 0;  // Fileline quiet w/o %% mark
    આભાસી શૂન્ય putsNoTracking(અચલ string& str) = 0;
    આભાસી શૂન્ય putsQuoted(અચલ string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    શૂન્ય iterateAndCommaConstNull(Ast_ગાંઠ* nodep) {
        માટે (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            જો (nodep->nextp()) puts(", ");
        }
    }
    શૂન્ય emitPacked() {
        માટે (Ast_ગાંઠ_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    શૂન્ય emitNodesWithText(Ast_ગાંઠ* nodesp, તાર્કિક tracking, અચલ std::string& separator) {
        માટે (Ast_ગાંઠ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            જો (અચલ AstText* અચલ textp = VN_CAST(nodep, Text)) {
                જો (tracking) {
                    puts(textp->text());
                } નહીંતર {
                    putsNoTracking(textp->text());
                }
            } નહીંતર {
                iterateConst(nodep);
            }
            જો (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    શૂન્ય visit(AstPort* nodep) override {}
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    શૂન્ય visit(AstGenBlock* nodep) override {
        અચલ std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    શૂન્ય visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    શૂન્ય visit(AstGenCaseItem* nodep) override {
        જો (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } નહીંતર {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    શૂન્ય visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = સાચું;
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
    શૂન્ય visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        જો (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstBegin* nodep) override {
        જો (nodep->name() == "") {
            putbs("begin\n");
        } નહીંતર {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    શૂન્ય visit(AstFork* nodep) override {
        જો (nodep->name() == "") {
            putbs("fork\n");
        } નહીંતર {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    શૂન્ય visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstAlways* nodep) override {
        જો (અચલ AstAssignW* અચલ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            જો (!ap->nextp()) {
                putfs(nodep, "assign ");
                જો (Ast_ગાંઠ* અચલ tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                જો (!m_suppressSemi) puts(";\n");
                પાછા;
            }
        }
        putfs(nodep, "always ");
        જો (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        નહીંતર {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        જો (!m_suppressSemi) puts(";\n");
    }
    શૂન્ય visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        માટે (Ast_ગાંઠ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        જો (!m_suppressSemi) puts(";\n");
    }
    શૂન્ય visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        જો (!m_suppressSemi) puts(";\n");
    }
    શૂન્ય visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        જો (!m_suppressSemi) puts(";\n");
    }
    શૂન્ય visit(AstBreak*) override {
        putbs("break");
        જો (!m_suppressSemi) puts(";\n");
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        માટે (Ast_ગાંઠ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            જો (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        જો (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        જો (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    શૂન્ય visit(AstCase* nodep) override {
        putfs(nodep, "");
        જો (nodep->priorityPragma()) puts("priority ");
        જો (nodep->uniquePragma()) puts("unique ");
        જો (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        જો (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            જો (nodep->fullPragma()) puts(" full_case");
            જો (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    શૂન્ય visit(AstCaseItem* nodep) override {
        જો (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } નહીંતર {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstContinue*) override {
        putbs("continue");
        જો (!m_suppressSemi) puts(";\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Decl*) override {}  // N/A
    શૂન્ય visit(AstCoverInc*) override {}  // N/A
    શૂન્ય visit(AstCoverToggle*) override {}  // N/A

    શૂન્ય visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        જો (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    શૂન્ય visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        જો (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    શૂન્ય emitCoverageOption(Ast_ગાંઠ* nodep, તાર્કિક typeOption, VCoverOptionType optType,
                            Ast_ગાંઠ_પદાવલિ* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    શૂન્ય visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    શૂન્ય visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    શૂન્ય visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        જો (nodep->isArray()) {
            puts("[");
            જો (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        જો (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } નહીંતર જો (nodep->transp()) {
            puts(" = ");
            માટે (Ast_ગાંઠ* setp = nodep->transp(); setp; setp = setp->nextp()) {
                જો (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } નહીંતર જો (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } નહીંતર જો (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            માટે (Ast_ગાંઠ* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                જો (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        જો (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    શૂન્ય visit(AstCoverWith* nodep) override {
        અચલ તાર્કિક rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        જો (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        જો (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    શૂન્ય visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        જો (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        જો (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    શૂન્ય visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        જો (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    શૂન્ય visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    શૂન્ય visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    શૂન્ય visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        જો (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        જો (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    શૂન્ય visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    શૂન્ય visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        માટે (Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            જો (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        જો (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        જો (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } નહીંતર {
            puts(";\n");
        }
    }
    શૂન્ય visit(AstCoverTransSet* nodep) override {
        puts("(");
        માટે (Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            જો (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    શૂન્ય visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    શૂન્ય visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        જો (AstConst* અચલ lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->અંક().toString());
        } નહીંતર {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    શૂન્ય visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    શૂન્ય visit(Ast_કિંમત_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    શૂન્ય visit_ગાંઠ_Display(Ast_ગાંઠ* nodep, Ast_ગાંઠ* fileOrStrgp, અચલ string& text,
                          Ast_ગાંઠ* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        જો (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        માટે (Ast_ગાંઠ* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    શૂન્ય visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    શૂન્ય visit(AstDisplay* nodep) override {
        visit_ગાંઠ_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    શૂન્ય visit(AstElabDisplay* nodep) override {
        visit_ગાંઠ_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    શૂન્ય visit(AstFScanF* nodep) override {
        visit_ગાંઠ_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        visit_ગાંઠ_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    શૂન્ય visit(AstSFormat* nodep) override {
        visit_ગાંઠ_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    શૂન્ય visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    શૂન્ય visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    શૂન્ય visit(AstSFormatF* nodep) override {
        visit_ગાંઠ_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    શૂન્ય visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    શૂન્ય visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    શૂન્ય visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        જો (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    શૂન્ય visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        જો (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    શૂન્ય visit(AstJumpBlock* nodep) override {
        // Allocate label number
        અચલ size_t n = m_labelNumbers.size();
        અચલ તાર્કિક newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    શૂન્ય visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        અચલ auto it = m_labelNumbers.find(nodep->blockp());
        અચલ std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        જો (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        જો (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        જો (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        જો (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    શૂન્ય visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    શૂન્ય visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    શૂન્ય visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        જો (AstLoopTest* અચલ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            પાછા;
        }
        // Special case when the AstLoopTest is last for output readability
        જો (Ast_ગાંઠ* lastp = nodep->stmtsp()) {
            જ્યારે (Ast_ગાંઠ* અચલ nextp = lastp->nextp()) lastp = nextp;
            જો (AstLoopTest* અચલ testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                માટે (Ast_ગાંઠ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                પાછા;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        putfs(nodep, "");
        જો (અચલ AstIf* અચલ ifp = VN_CAST(nodep, If)) {
            જો (ifp->priorityPragma()) puts("priority ");
            જો (ifp->uniquePragma()) puts("unique ");
            જો (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        જો (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    શૂન્ય visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        જો (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            જો (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    શૂન્ય visit(AstSampled* nodep) override {
        જો (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    શૂન્ય visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    શૂન્ય visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        જો (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    શૂન્ય visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        જો (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    શૂન્ય visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    શૂન્ય visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    શૂન્ય visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        જો (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    શૂન્ય visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    શૂન્ય visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    શૂન્ય visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    શૂન્ય visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    શૂન્ય visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    શૂન્ય visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    શૂન્ય visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), સાચું, nodep->separator());
        puts(nodep->suffix());
    }
    શૂન્ય visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), સાચું, "");
        puts(");\n");
    }
    શૂન્ય visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), સાચું, "");
        puts(")");
    }
    શૂન્ય visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), ખોટું, "");
        puts(");\n");
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), ખોટું, "");
        puts(")");
    }

    શૂન્ય visit(AstScopeName* nodep) override {}
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    શૂન્ય visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    શૂન્ય visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    આભાસી શૂન્ય emitVerilogFormat(Ast_ગાંઠ* nodep, અચલ string& format, Ast_ગાંઠ* lhsp = nullptr,
                                   Ast_ગાંઠ* અચલ rhsp = nullptr, Ast_ગાંઠ* thsp = nullptr,
                                   Ast_ગાંઠ* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        તાર્કિક inPct = ખોટું;
        putbs("");
        માટે (અચલ અક્ષર c : format) {
            જો (!inPct && c == '%') {
                inPct = સાચું;
            } નહીંતર જો (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } નહીંતર {  // Format character
                inPct = ખોટું;
                પસંદગી (c) {
                સ્થિતિ '%': puts("%"); તોડો;
                સ્થિતિ 'f': putfs(nodep, ""); તોડો;
                સ્થિતિ 'k': putbs(""); તોડો;
                સ્થિતિ 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    તોડો;
                }
                સ્થિતિ 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    તોડો;
                }
                સ્થિતિ 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    તોડો;
                }
                સ્થિતિ 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    તોડો;
                }
                સ્થિતિ 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    તોડો;
                }
                મૂળભૂત: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); તોડો;
                }
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    શૂન્ય visit(Ast_ગાંઠ_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    શૂન્ય visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        જો (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    શૂન્ય visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        માટે (અચલ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            જો (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    શૂન્ય visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    શૂન્ય visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    શૂન્ય visit(AstRange* nodep) override {
        puts("[");
        જો (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->ડાબું_Const()));
            puts(":");
            puts(cvtToStr(nodep->જમણું_Const()));
        } નહીંતર {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    શૂન્ય visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    શૂન્ય visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        પૂર્ણાંક offset = 0;
        Ast_ગાંઠ_DType* અચલ dtypep = nodep->fromp()->dtypep();
        જો (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* અચલ basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        જો (VN_IS(nodep->lsbp(), Const)) {
            જો (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } નહીંતર {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } નહીંતર {
            iterateAndNextConstNull(nodep->lsbp());
            જો (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    શૂન્ય visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    શૂન્ય visit(AstThisRef* /*nodep*/) override { puts("this"); }
    શૂન્ય visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        જો (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } નહીંતર જો (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } નહીંતર જો (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        જો (AstSampled* અચલ sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } નહીંતર {
            iterateAndNextConstNull(nodep->propp());
        }
        જો (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } નહીંતર {
            puts(");\n");
        }

        જો (અચલ AstAssert* અચલ assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } નહીંતર જો (અચલ AstAssertIntrinsic* અચલ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    શૂન્ય visit(AstAssocArrayDType* nodep) override {
        જો (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } નહીંતર {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ખોટું;
            puts("[");
            iterateConst(nodep->ચાવી_DTypep());
            puts("]");
            m_arrayPost = સાચું;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    શૂન્ય visit(AstBasicDType* nodep) override {
        જો (m_arrayPost) પાછા;
        putfs(nodep, nodep->prettyName());
        જો (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        જો (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) પાછા;
        emitPacked();
        જો (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } નહીંતર જો (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->ડાબું()));
            puts(":");
            puts(cvtToStr(nodep->જમણું()));
            puts("] ");
        }
    }
    શૂન્ય visit(AstConstDType* nodep) override {
        જો (m_arrayPost) પાછા;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    શૂન્ય visit(AstDynArrayDType* nodep) override {
        જો (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } નહીંતર {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    શૂન્ય visit(AstEnumDType* nodep) override {
        જો (m_arrayPost) પાછા;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        જો (Ast_ગાંઠ_Module* અચલ classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    શૂન્ય visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        જો (nodep->nextp()) puts(",");
        puts("\n");
    }
    શૂન્ય visit(Ast_ગાંઠ_Array_DType* nodep) override {
        જો (!m_arrayPost) {
            જો (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ધકેલો_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } નહીંતર {
            જો (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = ખોટું;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = સાચું;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        જો (m_arrayPost) {
            puts(" (");
            જો (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } નહીંતર {
                puts("????");
            }
            puts(")");
            પાછા;
        }
        puts(nodep->ifaceName());
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        જો (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } નહીંતર {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    શૂન્ય visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    શૂન્ય visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    શૂન્ય visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        જો (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } નહીંતર {
            puts("????");
        }
        puts(");\n");
    }
    શૂન્ય visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        જો (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = સાચું;
            iterateConst(nodep->varp());
        } નહીંતર {
            puts(nodep->prettyName());
        }
        જો (nodep->nextp()) puts(", ");
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        જો (m_arrayPost) પાછા;
        puts(nodep->verilogKwd() + " ");
        જો (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            માટે (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    શૂન્ય visit(AstMemberDType* nodep) override {
        જો (m_arrayPost) પાછા;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    શૂન્ય visit(AstQueueDType* nodep) override {
        જો (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } નહીંતર {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ખોટું;
            puts("[$");
            જો (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = સાચું;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        જો (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } નહીંતર {
            putfs(nodep, nodep->prettyName());
        }
        જો (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    શૂન્ય visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    શૂન્ય visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    શૂન્ય visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    શૂન્ય visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    શૂન્ય visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    શૂન્ય visit(AstPropSpec* nodep) override {
        જો (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            માટે (Ast_ગાંઠ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                જો (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        જો (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        જો (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        જો (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    શૂન્ય visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    શૂન્ય visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    શૂન્ય visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    શૂન્ય visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        જો (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    શૂન્ય visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = સાચું;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } નહીંતર {
            જો (nodep->varp()) {
                જો (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } નહીંતર {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } નહીંતર {
                putfs(nodep, nodep->name());
            }
        }
    }
    શૂન્ય visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        જો (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } નહીંતર {
            puts(nodep->prettyName());
        }
    }
    શૂન્ય visit(AstConst* nodep) override { putfs(nodep, nodep->અંક().ascii(m_prefixed, સાચું)); }
    શૂન્ય visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    શૂન્ય visit(Ast_ટોચ_Scope* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    શૂન્ય visit(AstVar* nodep) override {
        જો (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = ખોટું;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = સાચું;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    શૂન્ય visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    શૂન્ય visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        જો (nodep->isCycleDelay()) {
            puts("##");
        } નહીંતર {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = ખોટું;
        જો (Ast_ગાંઠ_પદાવલિ* અચલ fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } નહીંતર {
            iterateConst(nodep->lhsp());
        }
        જો (!m_suppressSemi) {
            puts(";\n");
        } નહીંતર {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    શૂન્ય visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        જો (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } નહીંતર {
            puts(";\n");
        }
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    શૂન્ય visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    શૂન્ય visit(AstVarScope*) override {}
    શૂન્ય visit(AstTraceDecl*) override {}
    શૂન્ય visit(AstTraceInc*) override {}
    // NOPs
    શૂન્ય visit(AstPragma*) override {}
    શૂન્ય visit(AstStmtPragma*) override {}
    શૂન્ય visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        જો (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

જાહેર:
    explicit EmitVBaseVisitorConst(તાર્કિક suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = મૂળભૂત;
};

//######################################################################
// Emit to an output file

વર્ગ EmitVFileVisitor final : જાહેર EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    શૂન્ય putsNoTracking(અચલ string& str) override { m_of.putsNoTracking(str); }
    શૂન્ય puts(અચલ string& str) override { m_of.puts(str); }
    શૂન્ય putbs(અચલ string& str) override { m_of.putbs(str); }
    શૂન્ય putfs(Ast_ગાંઠ*, અચલ string& str) override { putbs(str); }
    શૂન્ય putqs(Ast_ગાંઠ*, અચલ string& str) override { putbs(str); }

જાહેર:
    EmitVFileVisitor(Ast_ગાંઠ* nodep, V3OutVFile& of, તાર્કિક suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = મૂળભૂત;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

વર્ગ EmitVStreamVisitor final : જાહેર EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    અચલ તાર્કિક m_tracking;  // Use line tracking
    // METHODS
    શૂન્ય putsNoTracking(અચલ string& str) override { m_os.putsNoTracking(str); }
    શૂન્ય puts(અચલ string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    શૂન્ય putbs(અચલ string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    શૂન્ય putfs(Ast_ગાંઠ*, અચલ string& str) override { putbs(str); }
    શૂન્ય putqs(Ast_ગાંઠ*, અચલ string& str) override { putbs(str); }

જાહેર:
    EmitVStreamVisitor(અચલ Ast_ગાંઠ* nodep, std::ostream& os, તાર્કિક tracking, તાર્કિક suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ગાંઠ*>(nodep));
    }
    ~EmitVStreamVisitor() override = મૂળભૂત;
};

//######################################################################
// EmitV class functions

શૂન્ય V3EmitV::verilog_For_વૃક્ષ(અચલ Ast_ગાંઠ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ખોટું, ખોટું}; }
}

શૂન્ય V3EmitV::debug_Verilog_For_વૃક્ષ(અચલ Ast_ગાંઠ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ સાચું, સાચું}; }
}

std::string V3EmitV::debug_Verilog_For_વૃક્ષ(અચલ Ast_ગાંઠ* nodep) {
    std::stringstream ss;
    debug_Verilog_For_વૃક્ષ(nodep, ss);
    પાછા ss.str();
}

શૂન્ય V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    માટે (Ast_ગાંઠ_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ગાંઠ_File)) {
        AstVFile* અચલ vfilep = VN_CAST(filep, VFile);
        જો (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, ખોટું};
        }
    }
}

શૂન્ય V3EmitV::debugEmitV(અચલ string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, સાચું};
}
