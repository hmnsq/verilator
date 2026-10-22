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

#شامل "V3PchAstMT.h"

#شامل "V3EmitV.h"

#شامل "V3EmitCBase.h"

#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

رده EmitVBaseVisitorConst VL_NOT_FINAL : عمومی VNVisitorConst {
    // STATE - across all visitors
    ثابت منطقی m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_درخت* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقی m_suppressSemi = نادرست;  // Non-statement, don't print ;
    منطقی m_suppressVarSemi = نادرست;  // Suppress emitting semicolon for AstVars
    منطقی m_suppressSampled = نادرست;  // Suppress emitting sampled in assertion properties
    منطقی m_arrayPost = نادرست;  // Print array information that goes after identifier (vs after)
    منطقی m_prefixed = درست;  // Whether constants need to be prefixed
    std::deque<Ast_گره_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    مجازی تهی puts(ثابت string& str) = 0;
    مجازی تهی putbs(ثابت string& str) = 0;
    مجازی تهی putfs(Ast_گره* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    مجازی تهی putqs(Ast_گره* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    مجازی تهی putsNoTracking(ثابت string& str) = 0;
    مجازی تهی putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    تهی iterateAndCommaConstNull(Ast_گره* nodep) {
        برای (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            اگر (nodep->nextp()) puts(", ");
        }
    }
    تهی emitPacked() {
        برای (Ast_گره_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    تهی emitNodesWithText(Ast_گره* nodesp, منطقی tracking, ثابت std::string& separator) {
        برای (Ast_گره* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstText* ثابت textp = VN_CAST(nodep, Text)) {
                اگر (tracking) {
                    puts(textp->text());
                } وگرنه {
                    putsNoTracking(textp->text());
                }
            } وگرنه {
                iterateConst(nodep);
            }
            اگر (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    تهی visit(Ast_گره_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    تهی visit(AstPort* nodep) override {}
    تهی visit(Ast_گره_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    تهی visit(AstGenBlock* nodep) override {
        ثابت std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    تهی visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    تهی visit(AstGenCaseItem* nodep) override {
        اگر (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } وگرنه {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    تهی visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = درست;
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
    تهی visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        اگر (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    تهی visit(AstBegin* nodep) override {
        اگر (nodep->name() == "") {
            putbs("begin\n");
        } وگرنه {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    تهی visit(AstFork* nodep) override {
        اگر (nodep->name() == "") {
            putbs("fork\n");
        } وگرنه {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    تهی visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    تهی visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    تهی visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    تهی visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    تهی visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    تهی visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    تهی visit(AstAlways* nodep) override {
        اگر (ثابت AstAssignW* ثابت ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            اگر (!ap->nextp()) {
                putfs(nodep, "assign ");
                اگر (Ast_گره* ثابت tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                اگر (!m_suppressSemi) puts(";\n");
                بازگشت;
            }
        }
        putfs(nodep, "always ");
        اگر (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        وگرنه {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    تهی visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    تهی visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        اگر (!m_suppressSemi) puts(";\n");
    }
    تهی visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        برای (Ast_گره* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        اگر (!m_suppressSemi) puts(";\n");
    }
    تهی visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        اگر (!m_suppressSemi) puts(";\n");
    }
    تهی visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        اگر (!m_suppressSemi) puts(";\n");
    }
    تهی visit(AstBreak*) override {
        putbs("break");
        اگر (!m_suppressSemi) puts(";\n");
    }
    تهی visit(Ast_Sen_درخت* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        برای (Ast_گره* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            اگر (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    تهی visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        اگر (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        اگر (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    تهی visit(AstCase* nodep) override {
        putfs(nodep, "");
        اگر (nodep->priorityPragma()) puts("priority ");
        اگر (nodep->uniquePragma()) puts("unique ");
        اگر (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        اگر (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            اگر (nodep->fullPragma()) puts(" full_case");
            اگر (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    تهی visit(AstCaseItem* nodep) override {
        اگر (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } وگرنه {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    تهی visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    تهی visit(AstContinue*) override {
        putbs("continue");
        اگر (!m_suppressSemi) puts(";\n");
    }
    تهی visit(Ast_گره_Cover_Decl*) override {}  // N/A
    تهی visit(AstCoverInc*) override {}  // N/A
    تهی visit(AstCoverToggle*) override {}  // N/A

    تهی visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        اگر (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    تهی visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        اگر (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    تهی emitCoverageOption(Ast_گره* nodep, منطقی typeOption, VCoverOptionType optType,
                            Ast_گره_عبارت* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    تهی visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    تهی visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    تهی visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        اگر (nodep->isArray()) {
            puts("[");
            اگر (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        اگر (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } وگرنه اگر (nodep->transp()) {
            puts(" = ");
            برای (Ast_گره* setp = nodep->transp(); setp; setp = setp->nextp()) {
                اگر (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } وگرنه اگر (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } وگرنه اگر (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            برای (Ast_گره* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                اگر (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        اگر (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    تهی visit(AstCoverWith* nodep) override {
        ثابت منطقی rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        اگر (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        اگر (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    تهی visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        اگر (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        اگر (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    تهی visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        اگر (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    تهی visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    تهی visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    تهی visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        اگر (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        اگر (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    تهی visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    تهی visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        برای (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        اگر (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        اگر (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } وگرنه {
            puts(";\n");
        }
    }
    تهی visit(AstCoverTransSet* nodep) override {
        puts("(");
        برای (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    تهی visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    تهی visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        اگر (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->رقم().toString());
        } وگرنه {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    تهی visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    تهی visit(Ast_مقدار_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    تهی visit_گره_Display(Ast_گره* nodep, Ast_گره* fileOrStrgp, ثابت string& text,
                          Ast_گره* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        برای (Ast_گره* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    تهی visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    تهی visit(AstDisplay* nodep) override {
        visit_گره_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    تهی visit(AstElabDisplay* nodep) override {
        visit_گره_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    تهی visit(AstFScanF* nodep) override {
        visit_گره_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    تهی visit(AstSScanF* nodep) override {
        visit_گره_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    تهی visit(AstSFormat* nodep) override {
        visit_گره_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    تهی visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    تهی visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    تهی visit(AstSFormatF* nodep) override {
        visit_گره_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    تهی visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    تهی visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    تهی visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    تهی visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    تهی visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ثابت size_t n = m_labelNumbers.size();
        ثابت منطقی newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    تهی visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ثابت auto it = m_labelNumbers.find(nodep->blockp());
        ثابت std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    تهی visit(Ast_گره_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        اگر (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        اگر (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        اگر (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    تهی visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    تهی visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    تهی visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        اگر (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            بازگشت;
        }
        // Special case when the AstLoopTest is last for output readability
        اگر (Ast_گره* lastp = nodep->stmtsp()) {
            وقتی (Ast_گره* ثابت nextp = lastp->nextp()) lastp = nextp;
            اگر (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                برای (Ast_گره* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                بازگشت;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    تهی visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    تهی visit(Ast_گره_If* nodep) override {
        putfs(nodep, "");
        اگر (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            اگر (ifp->priorityPragma()) puts("priority ");
            اگر (ifp->uniquePragma()) puts("unique ");
            اگر (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        اگر (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    تهی visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        اگر (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            اگر (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    تهی visit(AstSampled* nodep) override {
        اگر (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    تهی visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    تهی visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        اگر (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    تهی visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        اگر (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    تهی visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    تهی visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    تهی visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        اگر (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    تهی visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    تهی visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    تهی visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    تهی visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    تهی visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    تهی visit(Ast_Stmt_عبارت* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    تهی visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    تهی visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), درست, nodep->separator());
        puts(nodep->suffix());
    }
    تهی visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), درست, "");
        puts(");\n");
    }
    تهی visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), درست, "");
        puts(")");
    }
    تهی visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), نادرست, "");
        puts(");\n");
    }
    تهی visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), نادرست, "");
        puts(")");
    }

    تهی visit(AstScopeName* nodep) override {}
    تهی visit(Ast_عبارت_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    تهی visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    تهی visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    مجازی تهی emitVerilogFormat(Ast_گره* nodep, ثابت string& format, Ast_گره* lhsp = nullptr,
                                   Ast_گره* ثابت rhsp = nullptr, Ast_گره* thsp = nullptr,
                                   Ast_گره* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        منطقی inPct = نادرست;
        putbs("");
        برای (ثابت نویسه c : format) {
            اگر (!inPct && c == '%') {
                inPct = درست;
            } وگرنه اگر (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } وگرنه {  // Format character
                inPct = نادرست;
                انتخاب (c) {
                حالت '%': puts("%"); بشکن;
                حالت 'f': putfs(nodep, ""); بشکن;
                حالت 'k': putbs(""); بشکن;
                حالت 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    بشکن;
                }
                حالت 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    بشکن;
                }
                حالت 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    بشکن;
                }
                حالت 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    بشکن;
                }
                حالت 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    بشکن;
                }
                پیشفرض: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); بشکن;
                }
            }
        }
    }

    تهی visit(Ast_گره_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    تهی visit(Ast_گره_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    تهی visit(Ast_گره_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    تهی visit(Ast_گره_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    تهی visit(Ast_گره_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    تهی visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    تهی visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    تهی visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        اگر (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    تهی visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        برای (ثابت AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            اگر (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    تهی visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    تهی visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    تهی visit(AstRange* nodep) override {
        puts("[");
        اگر (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->چپ_Const()));
            puts(":");
            puts(cvtToStr(nodep->راست_Const()));
        } وگرنه {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    تهی visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    تهی visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        صحیح offset = 0;
        Ast_گره_DType* ثابت dtypep = nodep->fromp()->dtypep();
        اگر (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        اگر (VN_IS(nodep->lsbp(), Const)) {
            اگر (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } وگرنه {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } وگرنه {
            iterateAndNextConstNull(nodep->lsbp());
            اگر (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    تهی visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    تهی visit(AstThisRef* /*nodep*/) override { puts("this"); }
    تهی visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    تهی visit(Ast_گره_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        اگر (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } وگرنه اگر (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } وگرنه اگر (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        اگر (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } وگرنه {
            iterateAndNextConstNull(nodep->propp());
        }
        اگر (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } وگرنه {
            puts(");\n");
        }

        اگر (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } وگرنه اگر (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    تهی visit(AstAssocArrayDType* nodep) override {
        اگر (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وگرنه {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = نادرست;
            puts("[");
            iterateConst(nodep->کلید_DTypep());
            puts("]");
            m_arrayPost = درست;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    تهی visit(AstBasicDType* nodep) override {
        اگر (m_arrayPost) بازگشت;
        putfs(nodep, nodep->prettyName());
        اگر (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        اگر (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) بازگشت;
        emitPacked();
        اگر (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } وگرنه اگر (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->چپ()));
            puts(":");
            puts(cvtToStr(nodep->راست()));
            puts("] ");
        }
    }
    تهی visit(AstConstDType* nodep) override {
        اگر (m_arrayPost) بازگشت;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    تهی visit(AstDynArrayDType* nodep) override {
        اگر (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وگرنه {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    تهی visit(AstEnumDType* nodep) override {
        اگر (m_arrayPost) بازگشت;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    تهی visit(AstEnumItemRef* nodep) override {
        اگر (Ast_گره_Module* ثابت classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    تهی visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        اگر (nodep->nextp()) puts(",");
        puts("\n");
    }
    تهی visit(Ast_گره_Array_DType* nodep) override {
        اگر (!m_arrayPost) {
            اگر (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.افزودن_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } وگرنه {
            اگر (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = نادرست;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = درست;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    تهی visit(AstIfaceRefDType* nodep) override {
        اگر (m_arrayPost) {
            puts(" (");
            اگر (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } وگرنه {
                puts("????");
            }
            puts(")");
            بازگشت;
        }
        puts(nodep->ifaceName());
    }
    تهی visit(AstRefDType* nodep) override {
        اگر (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } وگرنه {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    تهی visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    تهی visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    تهی visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        اگر (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } وگرنه {
            puts("????");
        }
        puts(");\n");
    }
    تهی visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        اگر (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = درست;
            iterateConst(nodep->varp());
        } وگرنه {
            puts(nodep->prettyName());
        }
        اگر (nodep->nextp()) puts(", ");
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        اگر (m_arrayPost) بازگشت;
        puts(nodep->verilogKwd() + " ");
        اگر (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            برای (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    تهی visit(AstMemberDType* nodep) override {
        اگر (m_arrayPost) بازگشت;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    تهی visit(AstQueueDType* nodep) override {
        اگر (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وگرنه {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = نادرست;
            puts("[$");
            اگر (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = درست;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } وگرنه {
            putfs(nodep, nodep->prettyName());
        }
        اگر (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    تهی visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    تهی visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    تهی visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    تهی visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    تهی visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    تهی visit(AstPropSpec* nodep) override {
        اگر (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            برای (Ast_گره* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                اگر (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        اگر (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        اگر (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        اگر (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    تهی visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    تهی visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    تهی visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    تهی visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        اگر (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    تهی visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = درست;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    تهی visit(AstVarRef* nodep) override {
        اگر (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } وگرنه {
            اگر (nodep->varp()) {
                اگر (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } وگرنه {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } وگرنه {
                putfs(nodep, nodep->name());
            }
        }
    }
    تهی visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        اگر (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } وگرنه {
            puts(nodep->prettyName());
        }
    }
    تهی visit(AstConst* nodep) override { putfs(nodep, nodep->رقم().ascii(m_prefixed, درست)); }
    تهی visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    تهی visit(Ast_بالاترین_Scope* nodep) override { iterateChildrenConst(nodep); }
    تهی visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    تهی visit(AstVar* nodep) override {
        اگر (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = نادرست;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = درست;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    تهی visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    تهی visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        اگر (nodep->isCycleDelay()) {
            puts("##");
        } وگرنه {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = نادرست;
        اگر (Ast_گره_عبارت* ثابت fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } وگرنه {
            iterateConst(nodep->lhsp());
        }
        اگر (!m_suppressSemi) {
            puts(";\n");
        } وگرنه {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    تهی visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        اگر (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } وگرنه {
            puts(";\n");
        }
    }
    تهی visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    تهی visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    تهی visit(AstVarScope*) override {}
    تهی visit(AstTraceDecl*) override {}
    تهی visit(AstTraceInc*) override {}
    // NOPs
    تهی visit(AstPragma*) override {}
    تهی visit(AstStmtPragma*) override {}
    تهی visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    تهی visit(Ast_گره* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        اگر (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

عمومی:
    explicit EmitVBaseVisitorConst(منطقی suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = پیشفرض;
};

//######################################################################
// Emit to an output file

رده EmitVFileVisitor final : عمومی EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    تهی putsNoTracking(ثابت string& str) override { m_of.putsNoTracking(str); }
    تهی puts(ثابت string& str) override { m_of.puts(str); }
    تهی putbs(ثابت string& str) override { m_of.putbs(str); }
    تهی putfs(Ast_گره*, ثابت string& str) override { putbs(str); }
    تهی putqs(Ast_گره*, ثابت string& str) override { putbs(str); }

عمومی:
    EmitVFileVisitor(Ast_گره* nodep, V3OutVFile& of, منطقی suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = پیشفرض;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

رده EmitVStreamVisitor final : عمومی EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    ثابت منطقی m_tracking;  // Use line tracking
    // METHODS
    تهی putsNoTracking(ثابت string& str) override { m_os.putsNoTracking(str); }
    تهی puts(ثابت string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    تهی putbs(ثابت string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    تهی putfs(Ast_گره*, ثابت string& str) override { putbs(str); }
    تهی putqs(Ast_گره*, ثابت string& str) override { putbs(str); }

عمومی:
    EmitVStreamVisitor(ثابت Ast_گره* nodep, std::ostream& os, منطقی tracking, منطقی suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_گره*>(nodep));
    }
    ~EmitVStreamVisitor() override = پیشفرض;
};

//######################################################################
// EmitV class functions

تهی V3EmitV::verilog_For_درخت(ثابت Ast_گره* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ نادرست, نادرست}; }
}

تهی V3EmitV::debug_Verilog_For_درخت(ثابت Ast_گره* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ درست, درست}; }
}

std::string V3EmitV::debug_Verilog_For_درخت(ثابت Ast_گره* nodep) {
    std::stringstream ss;
    debug_Verilog_For_درخت(nodep, ss);
    بازگشت ss.str();
}

تهی V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    برای (Ast_گره_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), گره_File)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        اگر (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, نادرست};
        }
    }
}

تهی V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, درست};
}
