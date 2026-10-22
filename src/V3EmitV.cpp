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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3EmitV.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

ವರ್ಗ EmitVBaseVisitorConst VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE - across all visitors
    ಧ್ರುವ ತಾರ್ಕಿಕ m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_ವೃಕ್ಷ* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    ತಾರ್ಕಿಕ m_suppressSemi = ಅಸತ್ಯ;  // Non-statement, don't print ;
    ತಾರ್ಕಿಕ m_suppressVarSemi = ಅಸತ್ಯ;  // Suppress emitting semicolon for AstVars
    ತಾರ್ಕಿಕ m_suppressSampled = ಅಸತ್ಯ;  // Suppress emitting sampled in assertion properties
    ತಾರ್ಕಿಕ m_arrayPost = ಅಸತ್ಯ;  // Print array information that goes after identifier (vs after)
    ತಾರ್ಕಿಕ m_prefixed = ಸತ್ಯ;  // Whether constants need to be prefixed
    std::deque<Ast_ಗಂಟು_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    ಆಭಾಸ ಶೂನ್ಯ puts(ಧ್ರುವ string& str) = 0;
    ಆಭಾಸ ಶೂನ್ಯ putbs(ಧ್ರುವ string& str) = 0;
    ಆಭಾಸ ಶೂನ್ಯ putfs(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& str) = 0;  // Fileline and node %% mark
    ಆಭಾಸ ಶೂನ್ಯ putqs(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& str) = 0;  // Fileline quiet w/o %% mark
    ಆಭಾಸ ಶೂನ್ಯ putsNoTracking(ಧ್ರುವ string& str) = 0;
    ಆಭಾಸ ಶೂನ್ಯ putsQuoted(ಧ್ರುವ string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    ಶೂನ್ಯ iterateAndCommaConstNull(Ast_ಗಂಟು* nodep) {
        ಪ್ರತಿ (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            ಯದಿ (nodep->nextp()) puts(", ");
        }
    }
    ಶೂನ್ಯ emitPacked() {
        ಪ್ರತಿ (Ast_ಗಂಟು_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    ಶೂನ್ಯ emitNodesWithText(Ast_ಗಂಟು* nodesp, ತಾರ್ಕಿಕ tracking, ಧ್ರುವ std::string& separator) {
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (ಧ್ರುವ AstText* ಧ್ರುವ textp = VN_CAST(nodep, Text)) {
                ಯದಿ (tracking) {
                    puts(textp->text());
                } ಅನ್ಯಥಾ {
                    putsNoTracking(textp->text());
                }
            } ಅನ್ಯಥಾ {
                iterateConst(nodep);
            }
            ಯದಿ (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    ಶೂನ್ಯ visit(AstPort* nodep) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        ಧ್ರುವ std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    ಶೂನ್ಯ visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ಶೂನ್ಯ visit(AstGenCaseItem* nodep) override {
        ಯದಿ (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } ಅನ್ಯಥಾ {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    ಶೂನ್ಯ visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ಸತ್ಯ;
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
    ಶೂನ್ಯ visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        ಯದಿ (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        ಯದಿ (nodep->name() == "") {
            putbs("begin\n");
        } ಅನ್ಯಥಾ {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        ಯದಿ (nodep->name() == "") {
            putbs("fork\n");
        } ಅನ್ಯಥಾ {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    ಶೂನ್ಯ visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (ಧ್ರುವ AstAssignW* ಧ್ರುವ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            ಯದಿ (!ap->nextp()) {
                putfs(nodep, "assign ");
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                ಯದಿ (!m_suppressSemi) puts(";\n");
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        putfs(nodep, "always ");
        ಯದಿ (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        ಅನ್ಯಥಾ {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        ಯದಿ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        ಯದಿ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        ಯದಿ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        ಯದಿ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ visit(AstBreak*) override {
        putbs("break");
        ಯದಿ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        ಪ್ರತಿ (Ast_ಗಂಟು* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            ಯದಿ (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        ಯದಿ (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        ಯದಿ (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    ಶೂನ್ಯ visit(AstCase* nodep) override {
        putfs(nodep, "");
        ಯದಿ (nodep->priorityPragma()) puts("priority ");
        ಯದಿ (nodep->uniquePragma()) puts("unique ");
        ಯದಿ (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        ಯದಿ (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            ಯದಿ (nodep->fullPragma()) puts(" full_case");
            ಯದಿ (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        ಯದಿ (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } ಅನ್ಯಥಾ {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstContinue*) override {
        putbs("continue");
        ಯದಿ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Decl*) override {}  // N/A
    ಶೂನ್ಯ visit(AstCoverInc*) override {}  // N/A
    ಶೂನ್ಯ visit(AstCoverToggle*) override {}  // N/A

    ಶೂನ್ಯ visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        ಯದಿ (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    ಶೂನ್ಯ visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        ಯದಿ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    ಶೂನ್ಯ emitCoverageOption(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ typeOption, VCoverOptionType optType,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ಶೂನ್ಯ visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ಶೂನ್ಯ visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        ಯದಿ (nodep->isArray()) {
            puts("[");
            ಯದಿ (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        ಯದಿ (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->transp()) {
            puts(" = ");
            ಪ್ರತಿ (Ast_ಗಂಟು* setp = nodep->transp(); setp; setp = setp->nextp()) {
                ಯದಿ (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            ಪ್ರತಿ (Ast_ಗಂಟು* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                ಯದಿ (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        ಯದಿ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstCoverWith* nodep) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        ಯದಿ (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        ಯದಿ (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        ಯದಿ (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        ಯದಿ (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    ಶೂನ್ಯ visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        ಯದಿ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    ಶೂನ್ಯ visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        ಯದಿ (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        ಯದಿ (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    ಶೂನ್ಯ visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    ಶೂನ್ಯ visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        ಯದಿ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        ಯದಿ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } ಅನ್ಯಥಾ {
            puts(";\n");
        }
    }
    ಶೂನ್ಯ visit(AstCoverTransSet* nodep) override {
        puts("(");
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    ಶೂನ್ಯ visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        ಯದಿ (AstConst* ಧ್ರುವ lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->ಅಂಕೆ().toString());
        } ಅನ್ಯಥಾ {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    ಶೂನ್ಯ visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    ಶೂನ್ಯ visit(Ast_ಮೌಲ್ಯ_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    ಶೂನ್ಯ visit_ಗಂಟು_Display(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* fileOrStrgp, ಧ್ರುವ string& text,
                          Ast_ಗಂಟು* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಯದಿ (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        ಪ್ರತಿ (Ast_ಗಂಟು* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        visit_ಗಂಟು_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ಶೂನ್ಯ visit(AstElabDisplay* nodep) override {
        visit_ಗಂಟು_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ಶೂನ್ಯ visit(AstFScanF* nodep) override {
        visit_ಗಂಟು_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    ಶೂನ್ಯ visit(AstSScanF* nodep) override {
        visit_ಗಂಟು_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    ಶೂನ್ಯ visit(AstSFormat* nodep) override {
        visit_ಗಂಟು_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    ಶೂನ್ಯ visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    ಶೂನ್ಯ visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        visit_ಗಂಟು_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    ಶೂನ್ಯ visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಯದಿ (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಯದಿ (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ಧ್ರುವ size_t n = m_labelNumbers.size();
        ಧ್ರುವ ತಾರ್ಕಿಕ newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ಧ್ರುವ auto it = m_labelNumbers.find(nodep->blockp());
        ಧ್ರುವ std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಯದಿ (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        ಯದಿ (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        ಯದಿ (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        ಯದಿ (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Special case when the AstLoopTest is last for output readability
        ಯದಿ (Ast_ಗಂಟು* lastp = nodep->stmtsp()) {
            ಯಾವತ್ (Ast_ಗಂಟು* ಧ್ರುವ nextp = lastp->nextp()) lastp = nextp;
            ಯದಿ (AstLoopTest* ಧ್ರುವ testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                ಪ್ರತಿ (Ast_ಗಂಟು* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        putfs(nodep, "");
        ಯದಿ (ಧ್ರುವ AstIf* ಧ್ರುವ ifp = VN_CAST(nodep, If)) {
            ಯದಿ (ifp->priorityPragma()) puts("priority ");
            ಯದಿ (ifp->uniquePragma()) puts("unique ");
            ಯದಿ (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        ಯದಿ (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        ಯದಿ (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            ಯದಿ (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstSampled* nodep) override {
        ಯದಿ (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    ಶೂನ್ಯ visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        ಯದಿ (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        ಯದಿ (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        ಯದಿ (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    ಶೂನ್ಯ visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    ಶೂನ್ಯ visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ಶೂನ್ಯ visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), ಸತ್ಯ, nodep->separator());
        puts(nodep->suffix());
    }
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), ಸತ್ಯ, "");
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), ಸತ್ಯ, "");
        puts(")");
    }
    ಶೂನ್ಯ visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), ಅಸತ್ಯ, "");
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), ಅಸತ್ಯ, "");
        puts(")");
    }

    ಶೂನ್ಯ visit(AstScopeName* nodep) override {}
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    ಶೂನ್ಯ visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    ಆಭಾಸ ಶೂನ್ಯ emitVerilogFormat(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& format, Ast_ಗಂಟು* lhsp = nullptr,
                                   Ast_ಗಂಟು* ಧ್ರುವ rhsp = nullptr, Ast_ಗಂಟು* thsp = nullptr,
                                   Ast_ಗಂಟು* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        ತಾರ್ಕಿಕ inPct = ಅಸತ್ಯ;
        putbs("");
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : format) {
            ಯದಿ (!inPct && c == '%') {
                inPct = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } ಅನ್ಯಥಾ {  // Format character
                inPct = ಅಸತ್ಯ;
                ವಿಕಲ್ಪ (c) {
                ಪಕ್ಷ '%': puts("%"); ಭಙ್ಗ;
                ಪಕ್ಷ 'f': putfs(nodep, ""); ಭಙ್ಗ;
                ಪಕ್ಷ 'k': putbs(""); ಭಙ್ಗ;
                ಪಕ್ಷ 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    ಭಙ್ಗ;
                }
                ಸ್ವಭಾವ: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); ಭಙ್ಗ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ಶೂನ್ಯ visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        ಯದಿ (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    ಶೂನ್ಯ visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        ಪ್ರತಿ (ಧ್ರುವ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            ಯದಿ (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    ಶೂನ್ಯ visit(AstRange* nodep) override {
        puts("[");
        ಯದಿ (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->ಎಡ_Const()));
            puts(":");
            puts(cvtToStr(nodep->ಬಲ_Const()));
        } ಅನ್ಯಥಾ {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    ಶೂನ್ಯ visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        ಪೂರ್ಣಾಙ್ಕ offset = 0;
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->fromp()->dtypep();
        ಯದಿ (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ಧ್ರುವ basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        ಯದಿ (VN_IS(nodep->lsbp(), Const)) {
            ಯದಿ (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } ಅನ್ಯಥಾ {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } ಅನ್ಯಥಾ {
            iterateAndNextConstNull(nodep->lsbp());
            ಯದಿ (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    ಶೂನ್ಯ visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    ಶೂನ್ಯ visit(AstThisRef* /*nodep*/) override { puts("this"); }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        ಯದಿ (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        ಯದಿ (AstSampled* ಧ್ರುವ sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } ಅನ್ಯಥಾ {
            iterateAndNextConstNull(nodep->propp());
        }
        ಯದಿ (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } ಅನ್ಯಥಾ {
            puts(");\n");
        }

        ಯದಿ (ಧ್ರುವ AstAssert* ಧ್ರುವ assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstAssertIntrinsic* ಧ್ರುವ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    ಶೂನ್ಯ visit(AstAssocArrayDType* nodep) override {
        ಯದಿ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ಅಸತ್ಯ;
            puts("[");
            iterateConst(nodep->ಕೀಲಿ_DTypep());
            puts("]");
            m_arrayPost = ಸತ್ಯ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ visit(AstBasicDType* nodep) override {
        ಯದಿ (m_arrayPost) ಪ್ರತ್ಯಾಗಮ;
        putfs(nodep, nodep->prettyName());
        ಯದಿ (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        ಯದಿ (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) ಪ್ರತ್ಯಾಗಮ;
        emitPacked();
        ಯದಿ (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->ಎಡ()));
            puts(":");
            puts(cvtToStr(nodep->ಬಲ()));
            puts("] ");
        }
    }
    ಶೂನ್ಯ visit(AstConstDType* nodep) override {
        ಯದಿ (m_arrayPost) ಪ್ರತ್ಯಾಗಮ;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    ಶೂನ್ಯ visit(AstDynArrayDType* nodep) override {
        ಯದಿ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ಅನ್ಯಥಾ {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ visit(AstEnumDType* nodep) override {
        ಯದಿ (m_arrayPost) ಪ್ರತ್ಯಾಗಮ;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    ಶೂನ್ಯ visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        ಯದಿ (nodep->nextp()) puts(",");
        puts("\n");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Array_DType* nodep) override {
        ಯದಿ (!m_arrayPost) {
            ಯದಿ (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ತಳ್ಳು_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } ಅನ್ಯಥಾ {
            ಯದಿ (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = ಅಸತ್ಯ;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = ಸತ್ಯ;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        ಯದಿ (m_arrayPost) {
            puts(" (");
            ಯದಿ (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } ಅನ್ಯಥಾ {
                puts("????");
            }
            puts(")");
            ಪ್ರತ್ಯಾಗಮ;
        }
        puts(nodep->ifaceName());
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        ಯದಿ (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } ಅನ್ಯಥಾ {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    ಶೂನ್ಯ visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    ಶೂನ್ಯ visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        ಯದಿ (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } ಅನ್ಯಥಾ {
            puts("????");
        }
        puts(");\n");
    }
    ಶೂನ್ಯ visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        ಯದಿ (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = ಸತ್ಯ;
            iterateConst(nodep->varp());
        } ಅನ್ಯಥಾ {
            puts(nodep->prettyName());
        }
        ಯದಿ (nodep->nextp()) puts(", ");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಯದಿ (m_arrayPost) ಪ್ರತ್ಯಾಗಮ;
        puts(nodep->verilogKwd() + " ");
        ಯದಿ (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            ಪ್ರತಿ (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    ಶೂನ್ಯ visit(AstMemberDType* nodep) override {
        ಯದಿ (m_arrayPost) ಪ್ರತ್ಯಾಗಮ;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstQueueDType* nodep) override {
        ಯದಿ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ಅಸತ್ಯ;
            puts("[$");
            ಯದಿ (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = ಸತ್ಯ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } ಅನ್ಯಥಾ {
            putfs(nodep, nodep->prettyName());
        }
        ಯದಿ (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    ಶೂನ್ಯ visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    ಶೂನ್ಯ visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    ಶೂನ್ಯ visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    ಶೂನ್ಯ visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    ಶೂನ್ಯ visit(AstPropSpec* nodep) override {
        ಯದಿ (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            ಪ್ರತಿ (Ast_ಗಂಟು* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                ಯದಿ (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        ಯದಿ (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        ಯದಿ (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        ಯದಿ (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    ಶೂನ್ಯ visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    ಶೂನ್ಯ visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    ಶೂನ್ಯ visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    ಶೂನ್ಯ visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        ಯದಿ (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    ಶೂನ್ಯ visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ಸತ್ಯ;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->varp()) {
                ಯದಿ (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } ಅನ್ಯಥಾ {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } ಅನ್ಯಥಾ {
                putfs(nodep, nodep->name());
            }
        }
    }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        ಯದಿ (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } ಅನ್ಯಥಾ {
            puts(nodep->prettyName());
        }
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override { putfs(nodep, nodep->ಅಂಕೆ().ascii(m_prefixed, ಸತ್ಯ)); }
    ಶೂನ್ಯ visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    ಶೂನ್ಯ visit(Ast_ತುದಿ_Scope* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = ಅಸತ್ಯ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = ಸತ್ಯ;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ಶೂನ್ಯ visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        ಯದಿ (nodep->isCycleDelay()) {
            puts("##");
        } ಅನ್ಯಥಾ {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = ಅಸತ್ಯ;
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } ಅನ್ಯಥಾ {
            iterateConst(nodep->lhsp());
        }
        ಯದಿ (!m_suppressSemi) {
            puts(";\n");
        } ಅನ್ಯಥಾ {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        ಯದಿ (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } ಅನ್ಯಥಾ {
            puts(";\n");
        }
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    ಶೂನ್ಯ visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    ಶೂನ್ಯ visit(AstVarScope*) override {}
    ಶೂನ್ಯ visit(AstTraceDecl*) override {}
    ಶೂನ್ಯ visit(AstTraceInc*) override {}
    // NOPs
    ಶೂನ್ಯ visit(AstPragma*) override {}
    ಶೂನ್ಯ visit(AstStmtPragma*) override {}
    ಶೂನ್ಯ visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        ಯದಿ (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

ಸಾರ್ವಜನಿಕ:
    explicit EmitVBaseVisitorConst(ತಾರ್ಕಿಕ suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = ಸ್ವಭಾವ;
};

//######################################################################
// Emit to an output file

ವರ್ಗ EmitVFileVisitor final : ಸಾರ್ವಜನಿಕ EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    ಶೂನ್ಯ putsNoTracking(ಧ್ರುವ string& str) override { m_of.putsNoTracking(str); }
    ಶೂನ್ಯ puts(ಧ್ರುವ string& str) override { m_of.puts(str); }
    ಶೂನ್ಯ putbs(ಧ್ರುವ string& str) override { m_of.putbs(str); }
    ಶೂನ್ಯ putfs(Ast_ಗಂಟು*, ಧ್ರುವ string& str) override { putbs(str); }
    ಶೂನ್ಯ putqs(Ast_ಗಂಟು*, ಧ್ರುವ string& str) override { putbs(str); }

ಸಾರ್ವಜನಿಕ:
    EmitVFileVisitor(Ast_ಗಂಟು* nodep, V3OutVFile& of, ತಾರ್ಕಿಕ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

ವರ್ಗ EmitVStreamVisitor final : ಸಾರ್ವಜನಿಕ EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    ಧ್ರುವ ತಾರ್ಕಿಕ m_tracking;  // Use line tracking
    // METHODS
    ಶೂನ್ಯ putsNoTracking(ಧ್ರುವ string& str) override { m_os.putsNoTracking(str); }
    ಶೂನ್ಯ puts(ಧ್ರುವ string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    ಶೂನ್ಯ putbs(ಧ್ರುವ string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    ಶೂನ್ಯ putfs(Ast_ಗಂಟು*, ಧ್ರುವ string& str) override { putbs(str); }
    ಶೂನ್ಯ putqs(Ast_ಗಂಟು*, ಧ್ರುವ string& str) override { putbs(str); }

ಸಾರ್ವಜನಿಕ:
    EmitVStreamVisitor(ಧ್ರುವ Ast_ಗಂಟು* nodep, std::ostream& os, ತಾರ್ಕಿಕ tracking, ತಾರ್ಕಿಕ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ಗಂಟು*>(nodep));
    }
    ~EmitVStreamVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// EmitV class functions

ಶೂನ್ಯ V3EmitV::verilog_For_ವೃಕ್ಷ(ಧ್ರುವ Ast_ಗಂಟು* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ಅಸತ್ಯ, ಅಸತ್ಯ}; }
}

ಶೂನ್ಯ V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(ಧ್ರುವ Ast_ಗಂಟು* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ಸತ್ಯ, ಸತ್ಯ}; }
}

std::string V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
    std::stringstream ss;
    debug_Verilog_For_ವೃಕ್ಷ(nodep, ss);
    ಪ್ರತ್ಯಾಗಮ ss.str();
}

ಶೂನ್ಯ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ಪ್ರತಿ (Ast_ಗಂಟು_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ಗಂಟು_File)) {
        AstVFile* ಧ್ರುವ vfilep = VN_CAST(filep, VFile);
        ಯದಿ (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, ಅಸತ್ಯ};
        }
    }
}

ಶೂನ್ಯ V3EmitV::debugEmitV(ಧ್ರುವ string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, ಸತ್ಯ};
}
