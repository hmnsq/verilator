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

#ضمن "V3PchAstMT.h"

#ضمن "V3EmitV.h"

#ضمن "V3EmitCBase.h"

#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

صنف EmitVBaseVisitorConst VL_NOT_FINAL : عام VNVisitorConst {
    // STATE - across all visitors
    ثابت منطقي m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_شجرة* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقي m_suppressSemi = خطأ;  // Non-statement, don't print ;
    منطقي m_suppressVarSemi = خطأ;  // Suppress emitting semicolon for AstVars
    منطقي m_suppressSampled = خطأ;  // Suppress emitting sampled in assertion properties
    منطقي m_arrayPost = خطأ;  // Print array information that goes after identifier (vs after)
    منطقي m_prefixed = true;  // Whether constants need to be prefixed
    std::deque<Ast_عقدة_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    virtual فراغ puts(ثابت string& str) = 0;
    virtual فراغ putbs(ثابت string& str) = 0;
    virtual فراغ putfs(Ast_عقدة* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    virtual فراغ putqs(Ast_عقدة* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    virtual فراغ putsNoTracking(ثابت string& str) = 0;
    virtual فراغ putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    فراغ iterateAndCommaConstNull(Ast_عقدة* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            إذا (nodep->nextp()) puts(", ");
        }
    }
    فراغ emitPacked() {
        لكل (Ast_عقدة_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    فراغ emitNodesWithText(Ast_عقدة* nodesp, منطقي tracking, ثابت std::string& separator) {
        لكل (Ast_عقدة* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstText* ثابت textp = VN_CAST(nodep, Text)) {
                إذا (tracking) {
                    puts(textp->text());
                } وإلا {
                    putsNoTracking(textp->text());
                }
            } وإلا {
                iterateConst(nodep);
            }
            إذا (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    فراغ visit(Ast_عقدة_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    فراغ visit(AstPort* nodep) override {}
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    فراغ visit(AstGenBlock* nodep) override {
        ثابت std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    فراغ visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ visit(AstGenCaseItem* nodep) override {
        إذا (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    فراغ visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = true;
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
    فراغ visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ visit(AstBegin* nodep) override {
        إذا (nodep->name() == "") {
            putbs("begin\n");
        } وإلا {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    فراغ visit(AstFork* nodep) override {
        إذا (nodep->name() == "") {
            putbs("fork\n");
        } وإلا {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    فراغ visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstAlways* nodep) override {
        إذا (ثابت AstAssignW* ثابت ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            إذا (!ap->nextp()) {
                putfs(nodep, "assign ");
                إذا (Ast_عقدة* ثابت tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                إذا (!m_suppressSemi) puts(";\n");
                إرجاع;
            }
        }
        putfs(nodep, "always ");
        إذا (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        وإلا {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ visit(Ast_عقدة_Assign* nodep) override {
        إذا (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        لكل (Ast_عقدة* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstBreak*) override {
        putbs("break");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(Ast_Sen_شجرة* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        لكل (Ast_عقدة* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            إذا (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    فراغ visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        إذا (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        إذا (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    فراغ visit(AstCase* nodep) override {
        putfs(nodep, "");
        إذا (nodep->priorityPragma()) puts("priority ");
        إذا (nodep->uniquePragma()) puts("unique ");
        إذا (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        إذا (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            إذا (nodep->fullPragma()) puts(" full_case");
            إذا (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ visit(AstCaseItem* nodep) override {
        إذا (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstContinue*) override {
        putbs("continue");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(Ast_عقدة_Cover_Decl*) override {}  // N/A
    فراغ visit(AstCoverInc*) override {}  // N/A
    فراغ visit(AstCoverToggle*) override {}  // N/A

    فراغ visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        إذا (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    فراغ visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        إذا (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    فراغ emitCoverageOption(Ast_عقدة* nodep, منطقي typeOption, VCoverOptionType optType,
                            Ast_عقدة_تعبير* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    فراغ visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        إذا (nodep->isArray()) {
            puts("[");
            إذا (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        إذا (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } وإلا إذا (nodep->transp()) {
            puts(" = ");
            لكل (Ast_عقدة* setp = nodep->transp(); setp; setp = setp->nextp()) {
                إذا (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } وإلا إذا (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } وإلا إذا (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            لكل (Ast_عقدة* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                إذا (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        إذا (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    فراغ visit(AstCoverWith* nodep) override {
        ثابت منطقي rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        إذا (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        إذا (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    فراغ visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        إذا (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        إذا (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    فراغ visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        إذا (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    فراغ visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    فراغ visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        إذا (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        إذا (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    فراغ visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    فراغ visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        لكل (Ast_عقدة* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        إذا (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        إذا (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } وإلا {
            puts(";\n");
        }
    }
    فراغ visit(AstCoverTransSet* nodep) override {
        puts("(");
        لكل (Ast_عقدة* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    فراغ visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    فراغ visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        إذا (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->رقم().toString());
        } وإلا {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    فراغ visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    فراغ visit(Ast_قيمة_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    فراغ visit_عقدة_Display(Ast_عقدة* nodep, Ast_عقدة* fileOrStrgp, ثابت string& text,
                          Ast_عقدة* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        لكل (Ast_عقدة* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    فراغ visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    فراغ visit(AstDisplay* nodep) override {
        visit_عقدة_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    فراغ visit(AstElabDisplay* nodep) override {
        visit_عقدة_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    فراغ visit(AstFScanF* nodep) override {
        visit_عقدة_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    فراغ visit(AstSScanF* nodep) override {
        visit_عقدة_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    فراغ visit(AstSFormat* nodep) override {
        visit_عقدة_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    فراغ visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    فراغ visit(AstSFormatF* nodep) override {
        visit_عقدة_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    فراغ visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    فراغ visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    فراغ visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    فراغ visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    فراغ visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ثابت size_t n = m_labelNumbers.size();
        ثابت منطقي newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    فراغ visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ثابت auto it = m_labelNumbers.find(nodep->blockp());
        ثابت std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    فراغ visit(Ast_عقدة_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        إذا (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        إذا (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        إذا (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    فراغ visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    فراغ visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    فراغ visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            إرجاع;
        }
        // Special case when the AstLoopTest is last for output readability
        إذا (Ast_عقدة* lastp = nodep->stmtsp()) {
            بينما (Ast_عقدة* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                لكل (Ast_عقدة* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                إرجاع;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    فراغ visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    فراغ visit(Ast_عقدة_If* nodep) override {
        putfs(nodep, "");
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (ifp->priorityPragma()) puts("priority ");
            إذا (ifp->uniquePragma()) puts("unique ");
            إذا (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        إذا (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            إذا (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    فراغ visit(AstSampled* nodep) override {
        إذا (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    فراغ visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    فراغ visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    فراغ visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    فراغ visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    فراغ visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    فراغ visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    فراغ visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    فراغ visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    فراغ visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    فراغ visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ visit(Ast_Stmt_تعبير* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    فراغ visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    فراغ visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), true, nodep->separator());
        puts(nodep->suffix());
    }
    فراغ visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), true, "");
        puts(");\n");
    }
    فراغ visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), true, "");
        puts(")");
    }
    فراغ visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), خطأ, "");
        puts(");\n");
    }
    فراغ visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), خطأ, "");
        puts(")");
    }

    فراغ visit(AstScopeName* nodep) override {}
    فراغ visit(Ast_تعبير_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    فراغ visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    فراغ visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    virtual فراغ emitVerilogFormat(Ast_عقدة* nodep, ثابت string& format, Ast_عقدة* lhsp = nullptr,
                                   Ast_عقدة* ثابت rhsp = nullptr, Ast_عقدة* thsp = nullptr,
                                   Ast_عقدة* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        منطقي inPct = خطأ;
        putbs("");
        لكل (ثابت حرف c : format) {
            إذا (!inPct && c == '%') {
                inPct = true;
            } وإلا إذا (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } وإلا {  // Format character
                inPct = خطأ;
                بدل (c) {
                حالة '%': puts("%"); اكسر;
                حالة 'f': putfs(nodep, ""); اكسر;
                حالة 'k': putbs(""); اكسر;
                حالة 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    اكسر;
                }
                حالة 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    اكسر;
                }
                حالة 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    اكسر;
                }
                حالة 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    اكسر;
                }
                حالة 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    اكسر;
                }
                افتراضي: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); اكسر;
                }
            }
        }
    }

    فراغ visit(Ast_عقدة_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    فراغ visit(Ast_عقدة_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    فراغ visit(Ast_عقدة_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    فراغ visit(Ast_عقدة_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    فراغ visit(Ast_عقدة_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    فراغ visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        إذا (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    فراغ visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        لكل (ثابت AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            إذا (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    فراغ visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    فراغ visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    فراغ visit(AstRange* nodep) override {
        puts("[");
        إذا (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->يسار_Const()));
            puts(":");
            puts(cvtToStr(nodep->يمين_Const()));
        } وإلا {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    فراغ visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    فراغ visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        صحيح offset = 0;
        Ast_عقدة_DType* ثابت dtypep = nodep->fromp()->dtypep();
        إذا (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        إذا (VN_IS(nodep->lsbp(), Const)) {
            إذا (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } وإلا {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } وإلا {
            iterateAndNextConstNull(nodep->lsbp());
            إذا (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    فراغ visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    فراغ visit(AstThisRef* /*nodep*/) override { puts("this"); }
    فراغ visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    فراغ visit(Ast_عقدة_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        إذا (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } وإلا إذا (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } وإلا إذا (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        إذا (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } وإلا {
            iterateAndNextConstNull(nodep->propp());
        }
        إذا (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } وإلا {
            puts(");\n");
        }

        إذا (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } وإلا إذا (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    فراغ visit(AstAssocArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[");
            iterateConst(nodep->مفتاح_DTypep());
            puts("]");
            m_arrayPost = true;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstBasicDType* nodep) override {
        إذا (m_arrayPost) إرجاع;
        putfs(nodep, nodep->prettyName());
        إذا (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        إذا (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) إرجاع;
        emitPacked();
        إذا (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } وإلا إذا (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->يسار()));
            puts(":");
            puts(cvtToStr(nodep->يمين()));
            puts("] ");
        }
    }
    فراغ visit(AstConstDType* nodep) override {
        إذا (m_arrayPost) إرجاع;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    فراغ visit(AstDynArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstEnumDType* nodep) override {
        إذا (m_arrayPost) إرجاع;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        إذا (Ast_عقدة_Module* ثابت classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    فراغ visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        إذا (nodep->nextp()) puts(",");
        puts("\n");
    }
    فراغ visit(Ast_عقدة_Array_DType* nodep) override {
        إذا (!m_arrayPost) {
            إذا (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ادفع_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } وإلا {
            إذا (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = خطأ;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = true;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstIfaceRefDType* nodep) override {
        إذا (m_arrayPost) {
            puts(" (");
            إذا (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } وإلا {
                puts("????");
            }
            puts(")");
            إرجاع;
        }
        puts(nodep->ifaceName());
    }
    فراغ visit(AstRefDType* nodep) override {
        إذا (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } وإلا {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    فراغ visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    فراغ visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        إذا (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } وإلا {
            puts("????");
        }
        puts(");\n");
    }
    فراغ visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        إذا (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = true;
            iterateConst(nodep->varp());
        } وإلا {
            puts(nodep->prettyName());
        }
        إذا (nodep->nextp()) puts(", ");
    }
    فراغ visit(Ast_عقدة_UOr_Struct_DType* nodep) override {
        إذا (m_arrayPost) إرجاع;
        puts(nodep->verilogKwd() + " ");
        إذا (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            لكل (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    فراغ visit(AstMemberDType* nodep) override {
        إذا (m_arrayPost) إرجاع;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    فراغ visit(AstQueueDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[$");
            إذا (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = true;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override {
        إذا (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } وإلا {
            putfs(nodep, nodep->prettyName());
        }
        إذا (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    فراغ visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    فراغ visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    فراغ visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    فراغ visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    فراغ visit(AstPropSpec* nodep) override {
        إذا (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            لكل (Ast_عقدة* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                إذا (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        إذا (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        إذا (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        إذا (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    فراغ visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    فراغ visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    فراغ visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    فراغ visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        إذا (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    فراغ visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = true;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    فراغ visit(AstVarRef* nodep) override {
        إذا (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } وإلا {
            إذا (nodep->varp()) {
                إذا (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } وإلا {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } وإلا {
                putfs(nodep, nodep->name());
            }
        }
    }
    فراغ visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        إذا (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } وإلا {
            puts(nodep->prettyName());
        }
    }
    فراغ visit(AstConst* nodep) override { putfs(nodep, nodep->رقم().ascii(m_prefixed, true)); }
    فراغ visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    فراغ visit(Ast_قمة_Scope* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstVar* nodep) override {
        إذا (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = خطأ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = true;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    فراغ visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    فراغ visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        إذا (nodep->isCycleDelay()) {
            puts("##");
        } وإلا {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = خطأ;
        إذا (Ast_عقدة_تعبير* ثابت fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } وإلا {
            iterateConst(nodep->lhsp());
        }
        إذا (!m_suppressSemi) {
            puts(";\n");
        } وإلا {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    فراغ visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        إذا (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } وإلا {
            puts(";\n");
        }
    }
    فراغ visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    فراغ visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    فراغ visit(AstVarScope*) override {}
    فراغ visit(AstTraceDecl*) override {}
    فراغ visit(AstTraceInc*) override {}
    // NOPs
    فراغ visit(AstPragma*) override {}
    فراغ visit(AstStmtPragma*) override {}
    فراغ visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    فراغ visit(Ast_عقدة* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        إذا (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

عام:
    explicit EmitVBaseVisitorConst(منطقي suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = افتراضي;
};

//######################################################################
// Emit to an output file

صنف EmitVFileVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_of.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override { m_of.puts(str); }
    فراغ putbs(ثابت string& str) override { m_of.putbs(str); }
    فراغ putfs(Ast_عقدة*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_عقدة*, ثابت string& str) override { putbs(str); }

عام:
    EmitVFileVisitor(Ast_عقدة* nodep, V3OutVFile& of, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = افتراضي;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

صنف EmitVStreamVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    ثابت منطقي m_tracking;  // Use line tracking
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_os.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    فراغ putbs(ثابت string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    فراغ putfs(Ast_عقدة*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_عقدة*, ثابت string& str) override { putbs(str); }

عام:
    EmitVStreamVisitor(ثابت Ast_عقدة* nodep, std::ostream& os, منطقي tracking, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_عقدة*>(nodep));
    }
    ~EmitVStreamVisitor() override = افتراضي;
};

//######################################################################
// EmitV class functions

فراغ V3EmitV::verilog_For_شجرة(ثابت Ast_عقدة* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ خطأ, خطأ}; }
}

فراغ V3EmitV::debug_Verilog_For_شجرة(ثابت Ast_عقدة* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ true, true}; }
}

std::string V3EmitV::debug_Verilog_For_شجرة(ثابت Ast_عقدة* nodep) {
    std::stringstream ss;
    debug_Verilog_For_شجرة(nodep, ss);
    إرجاع ss.str();
}

فراغ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (Ast_عقدة_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), عقدة_File)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        إذا (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, خطأ};
        }
    }
}

فراغ V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, true};
}
