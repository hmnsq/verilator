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
    Ast_Sen_ونه* m_sentreep = تش_اشاره;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقي m_suppressSemi = خطأ;  // Non-statement, don't print ;
    منطقي m_suppressVarSemi = خطأ;  // Suppress emitting semicolon for AstVars
    منطقي m_suppressSampled = خطأ;  // Suppress emitting sampled in assertion properties
    منطقي m_arrayPost = خطأ;  // Print array information that goes after identifier (vs after)
    منطقي m_prefixed = ریښتیا;  // Whether constants need to be prefixed
    std::deque<Ast_غوټه_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_بلاک*, اندازه_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    مجازي فراغ puts(ثابت string& str) = 0;
    مجازي فراغ putbs(ثابت string& str) = 0;
    مجازي فراغ putfs(Ast_غوټه* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    مجازي فراغ putqs(Ast_غوټه* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    مجازي فراغ putsNoTracking(ثابت string& str) = 0;
    مجازي فراغ putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_بهر_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    فراغ iterateAndCommaConstNull(Ast_غوټه* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            إذا (nodep->nextp()) puts(", ");
        }
    }
    فراغ emitPacked() {
        لكل (Ast_غوټه_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    فراغ emit_Nodes_With_متن(Ast_غوټه* nodesp, منطقي tracking, ثابت std::string& separator) {
        لكل (Ast_غوټه* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            إذا (ثابت Ast_متن* ثابت textp = VN_CAST(nodep, متن_2)) {
                إذا (tracking) {
                    puts(textp->متن());
                } وإلا {
                    putsNoTracking(textp->متن());
                }
            } وإلا {
                iterateConst(nodep);
            }
            إذا (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override { iterate_And_بل_Const_Null(nodep->modulesp()); }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::مختاړی_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    فراغ لیدنه(Ast_پورټ* nodep) override {}
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        ثابت std::string name = nodep->name().تشه() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    فراغ لیدنه(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_بل_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_بل_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ لیدنه(Ast_Gen_Case_توکی* nodep) override {
        إذا (nodep->condsp()) {
            iterate_And_بل_Const_Null(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        iterate_And_بل_Const_Null(nodep->itemsp());
    }
    فراغ لیدنه(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ریښتیا;
            iterate_And_بل_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_بل_Const_Null(nodep->condp());
            puts(";");
            iterate_And_بل_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_بل_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_بل_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_بل_Const_Null(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_بل_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstBegin* nodep) override {
        إذا (nodep->name() == "") {
            putbs("begin\n");
        } وإلا {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    فراغ لیدنه(AstFork* nodep) override {
        إذا (nodep->name() == "") {
            putbs("fork\n");
        } وإلا {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    فراغ لیدنه(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ لیدنه(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ لیدنه(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ لیدنه(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (ثابت Ast_ټاکل_W* ثابت ap = VN_CAST(nodep->stmtsp(), ټاکل_W)) {
            إذا (!ap->nextp()) {
                putfs(nodep, "assign ");
                إذا (Ast_غوټه* ثابت tcp = ap->timingControlp()) {
                    iterate_And_بل_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_بل_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_بل_Const_Null(ap->rhsp());
                إذا (!m_suppressSemi) puts(";\n");
                أرجع;
            }
        }
        putfs(nodep, "always ");
        إذا (m_sentreep) {
            iterate_And_بل_Const_Null(m_sentreep);
        }  // In active
        وإلا {
            iterate_And_بل_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (VN_IS(nodep, ټاکل_Force)) puts("force ");
        iterate_And_بل_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_بل_Const_Null(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ لیدنه(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        لكل (Ast_غوټه* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_بل_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_بل_Const_Null(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ لیدنه(AstRelease* nodep) override {
        puts("release ");
        iterate_And_بل_Const_Null(nodep->lhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ لیدنه(AstBreak*) override {
        putbs("break");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        لكل (Ast_غوټه* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            إذا (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        putfs(nodep, "");
        إذا (nodep->څنډه_Type() != VEdgeType::ET_بدل_شوی) puts(nodep->څنډه_Type().verilogKwd());
        إذا (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    فراغ لیدنه(AstCase* nodep) override {
        putfs(nodep, "");
        إذا (nodep->لومړیتوب_Pragma()) puts("priority ");
        إذا (nodep->uniquePragma()) puts("unique ");
        إذا (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_بل_Const_Null(nodep->exprp());
        puts(")\n");
        إذا (nodep->ډک_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            إذا (nodep->ډک_Pragma()) puts(" full_case");
            إذا (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_بل_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        إذا (nodep->condsp()) {
            iterate_And_بل_Const_Null(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstContinue*) override {
        putbs("continue");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ لیدنه(Ast_غوټه_Cover_Decl*) override {}  // N/A
    فراغ لیدنه(AstCoverInc*) override {}  // N/A
    فراغ لیدنه(AstCoverToggle*) override {}  // N/A

    فراغ لیدنه(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        إذا (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    فراغ لیدنه(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_بل_Const_Null(nodep->exprp());
        إذا (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_بل_Const_Null(nodep->optionsp());
            iterate_And_بل_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    فراغ emitCoverageOption(Ast_غوټه* nodep, منطقي typeOption, VCoverOptionType optType,
                            Ast_غوټه_بیان* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    فراغ لیدنه(Ast_Cg_Option_ټاکل* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ لیدنه(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ لیدنه(AstCoverBin* nodep) override {
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
            لكل (Ast_غوټه* setp = nodep->transp(); setp; setp = setp->nextp()) {
                إذا (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } وإلا إذا (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } وإلا إذا (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            لكل (Ast_غوټه* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
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
    فراغ لیدنه(AstCoverWith* nodep) override {
        ثابت منطقي rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        إذا (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        إذا (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    فراغ لیدنه(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        إذا (!nodep->name().تشه()) puts("." + nodep->name());
        puts(")");
        إذا (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    فراغ لیدنه(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        إذا (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    فراغ لیدنه(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ لیدنه(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    فراغ لیدنه(AstCoverCrossWith* nodep) override {
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
    فراغ لیدنه(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    فراغ لیدنه(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        لكل (Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
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
            iterate_And_بل_Const_Null(nodep->optionsp());
            iterate_And_بل_Const_Null(nodep->binsp());
            puts("}\n");
        } وإلا {
            puts(";\n");
        }
    }
    فراغ لیدنه(Ast_Cover_Trans_سېټ* nodep) override {
        puts("(");
        لكل (Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    فراغ لیدنه(Ast_Cover_Trans_توکی* nodep) override { iterateChildrenConst(nodep); }

    فراغ لیدنه(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        إذا (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->عدد().منزل_String());
        } وإلا {
            puts("string'(");
            iterate_And_بل_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    فراغ لیدنه(Ast_Test_Plus_دلیلونه* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    فراغ لیدنه(Ast_ارزښت_Plus_دلیلونه* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    فراغ لیدنه_غوټه_Display(Ast_غوټه* nodep, Ast_غوټه* fileOrStrgp, ثابت string& متن,
                          Ast_غوټه* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(متن);
        لكل (Ast_غوټه* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    فراغ لیدنه(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    فراغ لیدنه(AstDisplay* nodep) override {
        لیدنه_غوټه_Display(nodep, nodep->filep(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ لیدنه(AstElabDisplay* nodep) override {
        لیدنه_غوټه_Display(nodep, تش_اشاره, nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ لیدنه(AstFScanF* nodep) override {
        لیدنه_غوټه_Display(nodep, nodep->filep(), nodep->متن(), nodep->exprsp());
    }
    فراغ لیدنه(AstSScanF* nodep) override {
        لیدنه_غوټه_Display(nodep, nodep->fromp(), nodep->متن(), nodep->exprsp());
    }
    فراغ لیدنه(AstSFormat* nodep) override {
        لیدنه_غوټه_Display(nodep, nodep->lhsp(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ لیدنه(Ast_منزل_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ لیدنه(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    فراغ لیدنه(AstSFormatF* nodep) override {
        لیدنه_غوټه_Display(nodep, تش_اشاره, nodep->متن(), nodep->exprsp());
    }
    فراغ لیدنه(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_بل_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_بل_Const_Null(nodep->modep());
        puts(");\n");
    }
    فراغ لیدنه(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_بل_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    فراغ لیدنه(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterate_And_بل_Const_Null(nodep->filep());
        puts(");\n");
    }
    فراغ لیدنه(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterate_And_بل_Const_Null(nodep->filep());
        puts(");\n");
    }
    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        // Allocate label number
        ثابت اندازه_t n = m_labelNumbers.اندازه();
        ثابت منطقي new_ثبت = m_labelNumbers.emplace(nodep, n).دوهم;
        UASSERT_OBJ(new_ثبت, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::منزل_string(n) + "\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    فراغ لیدنه(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ثابت خپلکاری it = m_labelNumbers.پیدا_کړه(nodep->blockp());
        ثابت std::string label
            = it != m_labelNumbers.پای_ټکی() ? "label" + std::منزل_string(it->دوهم) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    فراغ لیدنه(Ast_غوټه_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filenamep()) iterate_And_بل_Const_Null(nodep->filenamep());
        putbs(", ");
        إذا (nodep->memp()) iterate_And_بل_Const_Null(nodep->memp());
        إذا (nodep->lsbp()) {
            putbs(", ");
            iterate_And_بل_Const_Null(nodep->lsbp());
        }
        إذا (nodep->msbp()) {
            putbs(", ");
            iterate_And_بل_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    فراغ لیدنه(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_بل_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    فراغ لیدنه(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_بل_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    فراغ لیدنه(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_بل_Const_Null(testp->nextp());
            puts("end\n");
            أرجع;
        }
        // Special case when the AstLoopTest is last for output readability
        إذا (Ast_غوټه* lastp = nodep->stmtsp()) {
            طالما (Ast_غوټه* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                لكل (Ast_غوټه* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                أرجع;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        iterate_And_بل_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    فراغ لیدنه(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_بل_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        putfs(nodep, "");
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (ifp->لومړیتوب_Pragma()) puts("priority ");
            إذا (ifp->uniquePragma()) puts("unique ");
            إذا (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_بل_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_بل_Const_Null(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_بل_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ لیدنه(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_بل_Const_Null(nodep->exprp());
        إذا (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_بل_Const_Null(nodep->ticksp());
            إذا (nodep->sentreep()) {
                puts(", ");
                iterate_And_بل_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    فراغ لیدنه(AstSampled* nodep) override {
        إذا (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_بل_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    فراغ لیدنه(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_بل_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ لیدنه(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_بل_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_بل_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ لیدنه(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_بل_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_بل_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ لیدنه(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_بل_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ لیدنه(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_بل_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ لیدنه(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_بل_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_بل_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ لیدنه(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_بل_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ لیدنه(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_بل_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    فراغ لیدنه(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    فراغ لیدنه(Ast_پای* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ لیدنه(Ast_پای_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    فراغ لیدنه(Ast_متن* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    فراغ لیدنه(Ast_متن_بلاک* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().تشه();
        puts(nodep->مختاړی());
        emit_Nodes_With_متن(nodep->nodesp(), ریښتیا, nodep->separator());
        puts(nodep->suffix());
    }
    فراغ لیدنه(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_متن(nodep->nodesp(), ریښتیا, "");
        puts(");\n");
    }
    فراغ لیدنه(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_متن(nodep->nodesp(), ریښتیا, "");
        puts(")");
    }
    فراغ لیدنه(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), خطأ, "");
        puts(");\n");
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), خطأ, "");
        puts(")");
    }

    فراغ لیدنه(AstScopeName* nodep) override {}
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_بل_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_بل_Const_Null(nodep->resultp());
        puts(");\n");
    }

    فراغ لیدنه(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    فراغ لیدنه(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    مجازي فراغ emitVerilogFormat(Ast_غوټه* nodep, ثابت string& format, Ast_غوټه* lhsp = تش_اشاره,
                                   Ast_غوټه* ثابت rhsp = تش_اشاره, Ast_غوټه* thsp = تش_اشاره,
                                   Ast_غوټه* fhsp = تش_اشاره) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        منطقي دننه_Pct = خطأ;
        putbs("");
        لكل (ثابت حرف c : format) {
            إذا (!دننه_Pct && c == '%') {
                دننه_Pct = ریښتیا;
            } وإلا إذا (!دننه_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } وإلا {  // Format character
                دننه_Pct = خطأ;
                بدل (c) {
                حالة '%': puts("%"); اكسر;
                حالة 'f': putfs(nodep, ""); اكسر;
                حالة 'k': putbs(""); اكسر;
                حالة 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بل_Const_Null(lhsp);
                    اكسر;
                }
                حالة 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بل_Const_Null(rhsp);
                    اكسر;
                }
                حالة 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بل_Const_Null(thsp);
                    اكسر;
                }
                حالة 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بل_Const_Null(fhsp);
                    اكسر;
                }
                حالة 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_بل_Const_Null(nodep->dtypep());
                    اكسر;
                }
                افتراضي: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); اكسر;
                }
            }
        }
    }

    فراغ لیدنه(Ast_غوټه_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    فراغ لیدنه(Ast_غوټه_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    فراغ لیدنه(Ast_غوټه_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    فراغ لیدنه(Ast_غوټه_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ لیدنه(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ لیدنه(Ast_Attr_د* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_بل_Const_Null(nodep->fromp());
        إذا (nodep->dimp()) {
            putbs(", ");
            iterate_And_بل_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    فراغ لیدنه(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        لكل (ثابت Ast_Cons_Array_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_توکی)) {
            إذا (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_منزل_Str(itemp->شاخص()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    فراغ لیدنه(AstCond* nodep) override {
        putbs("(");
        iterate_And_بل_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_بل_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_بل_Const_Null(nodep->elsep());
        puts(")");
    }
    فراغ لیدنه(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_بل_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_بل_Const_Null(nodep->rhsp());
        puts("]");
    }
    فراغ لیدنه(AstRange* nodep) override {
        puts("[");
        إذا (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_منزل_Str(nodep->کیڼ_Const()));
            puts(":");
            puts(cvt_منزل_Str(nodep->ښی_Const()));
        } وإلا {
            iterate_And_بل_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_بل_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    فراغ لیدنه(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    فراغ لیدنه(AstSel* nodep) override {
        iterate_And_بل_Const_Null(nodep->fromp());
        صحيح دننه_واټن = 0;
        Ast_غوټه_DType* ثابت dtypep = nodep->fromp()->dtypep();
        إذا (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            دننه_واټن = basicDtypep->lo();
        }
        puts("[");
        إذا (VN_IS(nodep->lsbp(), Const)) {
            إذا (nodep->پلنوالی_Const() == 1) {
                puts(cvt_منزل_Str(VN_AS(nodep->lsbp(), Const)->منزل_SInt() + دننه_واټن));
            } وإلا {
                puts(cvt_منزل_Str(VN_AS(nodep->lsbp(), Const)->منزل_SInt() + nodep->پلنوالی_Const() + دننه_واټن
                              - 1));
                puts(":");
                puts(cvt_منزل_Str(VN_AS(nodep->lsbp(), Const)->منزل_SInt() + دننه_واټن));
            }
        } وإلا {
            iterate_And_بل_Const_Null(nodep->lsbp());
            إذا (دننه_واټن != 0) {
                puts(" + ");
                puts(cvt_منزل_Str(دننه_واټن));
            }
            putfs(nodep, "+:");
            puts(cvt_منزل_Str(nodep->پلنوالی_Const()));
        }
        puts("]");
    }
    فراغ لیدنه(Ast_ټوټه_Sel* nodep) override {
        iterate_And_بل_Const_Null(nodep->fromp());
        puts(cvt_منزل_Str(nodep->declRange()));
    }
    فراغ لیدنه(AstThisRef* /*nodep*/) override { puts("this"); }
    فراغ لیدنه(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    فراغ لیدنه(Ast_غوټه_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        إذا (nodep->userType() == VAssertType::OBSERVED_DEFERRED_سمدستي) {
            puts("#0 ");
        } وإلا إذا (nodep->userType() == VAssertType::FINAL_DEFERRED_سمدستي) {
            puts("final ");
        } وإلا إذا (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        إذا (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_بل_Const_Null(sampledp->exprp());
        } وإلا {
            iterate_And_بل_Const_Null(nodep->propp());
        }
        إذا (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_بل_Const_Null(nodep->passsp());
            puts("end\n");
        } وإلا {
            puts(");\n");
        }

        إذا (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_بل_Const_Null(assertp->failsp());
            puts("end\n");
        } وإلا إذا (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_بل_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    فراغ لیدنه(AstAssocArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[");
            iterateConst(nodep->کلی_DTypep());
            puts("]");
            m_arrayPost = ریښتیا;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ لیدنه(AstBasicDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, nodep->prettyName());
        إذا (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        إذا (nodep->keyword().isIntNumeric() && !nodep->keyword().is_بټ_Logic()) أرجع;
        emitPacked();
        إذا (nodep->rangep()) {
            puts(" ");
            iterate_And_بل_Const_Null(nodep->rangep());
            puts(" ");
        } وإلا إذا (nodep->isRanged()) {
            puts(" [");
            puts(cvt_منزل_Str(nodep->کیڼ()));
            puts(":");
            puts(cvt_منزل_Str(nodep->ښی()));
            puts("] ");
        }
    }
    فراغ لیدنه(AstConstDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    فراغ لیدنه(AstDynArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ لیدنه(AstEnumDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_بل_Const_Null(nodep->itemsp());
        puts("}");
    }
    فراغ لیدنه(Ast_Enum_توکی_Ref* nodep) override {
        إذا (Ast_غوټه_Module* ثابت classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    فراغ لیدنه(Ast_Enum_توکی* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        إذا (nodep->nextp()) puts(",");
        puts("\n");
    }
    فراغ لیدنه(Ast_غوټه_Array_DType* nodep) override {
        إذا (!m_arrayPost) {
            إذا (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ټېل_شا(nodep);
            }
            iterateConst(nodep->subDTypep());
        } وإلا {
            إذا (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = خطأ;
                iterate_And_بل_Const_Null(nodep->rangep());
                m_arrayPost = ریښتیا;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        إذا (m_arrayPost) {
            puts(" (");
            إذا (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } وإلا {
                puts("????");
            }
            puts(")");
            أرجع;
        }
        puts(nodep->ifaceName());
    }
    فراغ لیدنه(AstRefDType* nodep) override {
        إذا (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } وإلا {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    فراغ لیدنه(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::مختاړی_Name_Protect(nodep->classp()));
    }
    فراغ لیدنه(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ لیدنه(AstModport* nodep) override {
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
    فراغ لیدنه(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        إذا (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = ریښتیا;
            iterateConst(nodep->varp());
        } وإلا {
            puts(nodep->prettyName());
        }
        إذا (nodep->nextp()) puts(", ");
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {
        إذا (m_arrayPost) أرجع;
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
    فراغ لیدنه(AstMemberDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    فراغ لیدنه(Ast_کتار_DType* nodep) override {
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
            m_arrayPost = ریښتیا;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } وإلا {
            putfs(nodep, nodep->prettyName());
        }
        إذا (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_بل_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    فراغ لیدنه(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_بل_Const_Null(nodep->argsp());
        puts(")");
    }
    فراغ لیدنه(AstArg* nodep) override { iterate_And_بل_Const_Null(nodep->exprp()); }
    فراغ لیدنه(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    فراغ لیدنه(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ لیدنه(Ast_Print_وخت_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    فراغ لیدنه(AstPropSpec* nodep) override {
        إذا (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            لكل (Ast_غوټه* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
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
    فراغ لیدنه(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    فراغ لیدنه(AstAbortOn* nodep) override {
        puts(nodep->ډول().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    فراغ لیدنه(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    فراغ لیدنه(AstPropAlways* nodep) override {
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
    فراغ لیدنه(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ریښتیا;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } وإلا {
            إذا (nodep->varp()) {
                إذا (nodep->self_اشاره_ګر().is_تشه()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } وإلا {
                    putfs(nodep, nodep->self_اشاره_ګر().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } وإلا {
                putfs(nodep, nodep->name());
            }
        }
    }
    فراغ لیدنه(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        إذا (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } وإلا {
            puts(nodep->prettyName());
        }
    }
    فراغ لیدنه(AstConst* nodep) override { putfs(nodep, nodep->عدد().ascii(m_prefixed, ریښتیا)); }
    فراغ لیدنه(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    فراغ لیدنه(Ast_سر_Scope* nodep) override { iterateChildrenConst(nodep); }
    فراغ لیدنه(AstScope* nodep) override { iterateChildrenConst(nodep); }
    فراغ لیدنه(AstVar* nodep) override {
        إذا (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = خطأ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = ریښتیا;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_بل_Const_Null(nodep->stmtsp());
    }
    فراغ لیدنه(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        إذا (nodep->is_دوره_Delay()) {
            puts("##");
        } وإلا {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = خطأ;
        إذا (Ast_غوټه_بیان* ثابت fallDelayp = nodep->fallDelay()) {
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
        iterate_And_بل_Const_Null(nodep->stmtsp());
    }
    فراغ لیدنه(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        إذا (nodep->stmtsp()) {
            puts(" ");
            iterate_And_بل_Const_Null(nodep->stmtsp());
        } وإلا {
            puts(";\n");
        }
    }
    فراغ لیدنه(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    فراغ لیدنه(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    فراغ لیدنه(AstVarScope*) override {}
    فراغ لیدنه(AstTraceDecl*) override {}
    فراغ لیدنه(AstTraceInc*) override {}
    // NOPs
    فراغ لیدنه(AstPragma*) override {}
    فراغ لیدنه(AstStmtPragma*) override {}
    فراغ لیدنه(AstCell*) override {}  // Handled outside the Visit class
    // Default
    فراغ لیدنه(Ast_غوټه* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        إذا (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

عام:
    څرګند EmitVBaseVisitorConst(منطقي suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = افتراضي;
};

//######################################################################
// Emit to an output file

صنف EmitVFileVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3_بهر_VFile& m_د;  // The output file
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_د.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override { m_د.puts(str); }
    فراغ putbs(ثابت string& str) override { m_د.putbs(str); }
    فراغ putfs(Ast_غوټه*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_غوټه*, ثابت string& str) override { putbs(str); }

عام:
    EmitVFileVisitor(Ast_غوټه* nodep, V3_بهر_VFile& د, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_د{د} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = افتراضي;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

صنف EmitVStreamVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3_بهر_بهیر m_os;  // The output stream formatter
    ثابت منطقي m_tracking;  // Use line tracking
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_os.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    فراغ putbs(ثابت string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    فراغ putfs(Ast_غوټه*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_غوټه*, ثابت string& str) override { putbs(str); }

عام:
    EmitVStreamVisitor(ثابت Ast_غوټه* nodep, std::ostream& os, منطقي tracking, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_بهر_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_غوټه*>(nodep));
    }
    ~EmitVStreamVisitor() override = افتراضي;
};

//######################################################################
// EmitV class functions

فراغ V3EmitV::verilog_For_ونه(ثابت Ast_غوټه* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ خطأ, خطأ}; }
}

فراغ V3EmitV::debug_Verilog_For_ونه(ثابت Ast_غوټه* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ریښتیا, ریښتیا}; }
}

std::string V3EmitV::debug_Verilog_For_ونه(ثابت Ast_غوټه* nodep) {
    std::stringstream ss;
    debug_Verilog_For_ونه(nodep, ss);
    أرجع ss.str();
}

فراغ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (Ast_غوټه_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), غوټه_File)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        إذا (vfilep && vfilep->tblockp()) {
            V3_بهر_VFile د{vfilep->name()};
            د.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), د, خطأ};
        }
    }
}

فراغ V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_بهر_VFile د{filename};
    EmitVFileVisitor{v3Global.rootp(), د, ریښتیا};
}
