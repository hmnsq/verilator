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
    Ast_Sen_وڻ* m_sentreep = خالي_اشارو;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقي m_suppressSemi = خطأ;  // Non-statement, don't print ;
    منطقي m_suppressVarSemi = خطأ;  // Suppress emitting semicolon for AstVars
    منطقي m_suppressSampled = خطأ;  // Suppress emitting sampled in assertion properties
    منطقي m_arrayPost = خطأ;  // Print array information that goes after identifier (vs after)
    منطقي m_prefixed = سچ;  // Whether constants need to be prefixed
    std::deque<Ast_ڳنڍ_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_بلاڪ*, سائيز_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    مجازي فراغ puts(ثابت string& str) = 0;
    مجازي فراغ putbs(ثابت string& str) = 0;
    مجازي فراغ putfs(Ast_ڳنڍ* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    مجازي فراغ putqs(Ast_ڳنڍ* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    مجازي فراغ putsNoTracking(ثابت string& str) = 0;
    مجازي فراغ putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_ٻاهريون_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    فراغ iterateAndCommaConstNull(Ast_ڳنڍ* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            إذا (nodep->nextp()) puts(", ");
        }
    }
    فراغ emitPacked() {
        لكل (Ast_ڳنڍ_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    فراغ emit_Nodes_With_متن(Ast_ڳنڍ* nodesp, منطقي tracking, ثابت std::string& separator) {
        لكل (Ast_ڳنڍ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
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
    فراغ گهمو(AstNetlist* nodep) override { iterate_And_ايندڙ_Const_Null(nodep->modulesp()); }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::اڳياڙي_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    فراغ گهمو(Ast_پورٽ* nodep) override {}
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        ثابت std::string name = nodep->name().خالي_جايون() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    فراغ گهمو(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_ايندڙ_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ گهمو(Ast_Gen_Case_شي* nodep) override {
        إذا (nodep->condsp()) {
            iterate_And_ايندڙ_Const_Null(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        iterate_And_ايندڙ_Const_Null(nodep->itemsp());
    }
    فراغ گهمو(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = سچ;
            iterate_And_ايندڙ_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_ايندڙ_Const_Null(nodep->condp());
            puts(";");
            iterate_And_ايندڙ_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_ايندڙ_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_ايندڙ_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstBegin* nodep) override {
        إذا (nodep->name() == "") {
            putbs("begin\n");
        } وإلا {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    فراغ گهمو(AstFork* nodep) override {
        إذا (nodep->name() == "") {
            putbs("fork\n");
        } وإلا {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    فراغ گهمو(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(AstAlways* nodep) override {
        إذا (ثابت Ast_سونپيو_W* ثابت ap = VN_CAST(nodep->stmtsp(), سونپيو_W)) {
            إذا (!ap->nextp()) {
                putfs(nodep, "assign ");
                إذا (Ast_ڳنڍ* ثابت tcp = ap->timingControlp()) {
                    iterate_And_ايندڙ_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_ايندڙ_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_ايندڙ_Const_Null(ap->rhsp());
                إذا (!m_suppressSemi) puts(";\n");
                أرجع;
            }
        }
        putfs(nodep, "always ");
        إذا (m_sentreep) {
            iterate_And_ايندڙ_Const_Null(m_sentreep);
        }  // In active
        وإلا {
            iterate_And_ايندڙ_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (VN_IS(nodep, سونپيو_Force)) puts("force ");
        iterate_And_ايندڙ_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_ايندڙ_Const_Null(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ گهمو(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        لكل (Ast_ڳنڍ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ گهمو(Ast_سونپيو_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_ايندڙ_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_ايندڙ_Const_Null(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ گهمو(AstRelease* nodep) override {
        puts("release ");
        iterate_And_ايندڙ_Const_Null(nodep->lhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ گهمو(AstBreak*) override {
        putbs("break");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        لكل (Ast_ڳنڍ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            إذا (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    فراغ گهمو(Ast_Sen_شي* nodep) override {
        putfs(nodep, "");
        إذا (nodep->ڪنارو_Type() != VEdgeType::ET_بدليو) puts(nodep->ڪنارو_Type().verilogKwd());
        إذا (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    فراغ گهمو(AstCase* nodep) override {
        putfs(nodep, "");
        إذا (nodep->ترجيح_Pragma()) puts("priority ");
        إذا (nodep->uniquePragma()) puts("unique ");
        إذا (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        puts(")\n");
        إذا (nodep->ڀريل_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            إذا (nodep->ڀريل_Pragma()) puts(" full_case");
            إذا (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_ايندڙ_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ گهمو(Ast_Case_شي* nodep) override {
        إذا (nodep->condsp()) {
            iterate_And_ايندڙ_Const_Null(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstContinue*) override {
        putbs("continue");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ گهمو(Ast_ڳنڍ_Cover_Decl*) override {}  // N/A
    فراغ گهمو(AstCoverInc*) override {}  // N/A
    فراغ گهمو(AstCoverToggle*) override {}  // N/A

    فراغ گهمو(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        إذا (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    فراغ گهمو(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        إذا (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_ايندڙ_Const_Null(nodep->optionsp());
            iterate_And_ايندڙ_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    فراغ emitCoverageOption(Ast_ڳنڍ* nodep, منطقي typeOption, VCoverOptionType optType,
                            Ast_ڳنڍ_اظهار* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    فراغ گهمو(Ast_Cg_Option_سونپيو* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ گهمو(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ گهمو(AstCoverBin* nodep) override {
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
            لكل (Ast_ڳنڍ* setp = nodep->transp(); setp; setp = setp->nextp()) {
                إذا (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } وإلا إذا (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } وإلا إذا (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            لكل (Ast_ڳنڍ* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
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
    فراغ گهمو(AstCoverWith* nodep) override {
        ثابت منطقي rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        إذا (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        إذا (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    فراغ گهمو(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        إذا (!nodep->name().خالي_جايون()) puts("." + nodep->name());
        puts(")");
        إذا (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    فراغ گهمو(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        إذا (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    فراغ گهمو(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ گهمو(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    فراغ گهمو(AstCoverCrossWith* nodep) override {
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
    فراغ گهمو(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    فراغ گهمو(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        لكل (Ast_ڳنڍ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
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
            iterate_And_ايندڙ_Const_Null(nodep->optionsp());
            iterate_And_ايندڙ_Const_Null(nodep->binsp());
            puts("}\n");
        } وإلا {
            puts(";\n");
        }
    }
    فراغ گهمو(Ast_Cover_Trans_سيٽ* nodep) override {
        puts("(");
        لكل (Ast_ڳنڍ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    فراغ گهمو(Ast_Cover_Trans_شي* nodep) override { iterateChildrenConst(nodep); }

    فراغ گهمو(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        إذا (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->انگ().منزل_String());
        } وإلا {
            puts("string'(");
            iterate_And_ايندڙ_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    فراغ گهمو(Ast_Test_Plus_دليلون* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    فراغ گهمو(Ast_قدر_Plus_دليلون* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    فراغ گهمو_ڳنڍ_Display(Ast_ڳنڍ* nodep, Ast_ڳنڍ* fileOrStrgp, ثابت string& متن,
                          Ast_ڳنڍ* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(متن);
        لكل (Ast_ڳنڍ* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    فراغ گهمو(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    فراغ گهمو(AstDisplay* nodep) override {
        گهمو_ڳنڍ_Display(nodep, nodep->filep(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ گهمو(AstElabDisplay* nodep) override {
        گهمو_ڳنڍ_Display(nodep, خالي_اشارو, nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ گهمو(AstFScanF* nodep) override {
        گهمو_ڳنڍ_Display(nodep, nodep->filep(), nodep->متن(), nodep->exprsp());
    }
    فراغ گهمو(AstSScanF* nodep) override {
        گهمو_ڳنڍ_Display(nodep, nodep->fromp(), nodep->متن(), nodep->exprsp());
    }
    فراغ گهمو(AstSFormat* nodep) override {
        گهمو_ڳنڍ_Display(nodep, nodep->lhsp(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ گهمو(Ast_منزل_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ گهمو(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    فراغ گهمو(AstSFormatF* nodep) override {
        گهمو_ڳنڍ_Display(nodep, خالي_اشارو, nodep->متن(), nodep->exprsp());
    }
    فراغ گهمو(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ايندڙ_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_ايندڙ_Const_Null(nodep->modep());
        puts(");\n");
    }
    فراغ گهمو(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ايندڙ_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    فراغ گهمو(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterate_And_ايندڙ_Const_Null(nodep->filep());
        puts(");\n");
    }
    فراغ گهمو(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterate_And_ايندڙ_Const_Null(nodep->filep());
        puts(");\n");
    }
    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        // Allocate label number
        ثابت سائيز_t n = m_labelNumbers.سائيز();
        ثابت منطقي new_داخلا = m_labelNumbers.emplace(nodep, n).ٻيو;
        UASSERT_OBJ(new_داخلا, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::منزل_string(n) + "\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    فراغ گهمو(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ثابت خودڪار it = m_labelNumbers.ڳوليو(nodep->blockp());
        ثابت std::string label
            = it != m_labelNumbers.پڇاڙي() ? "label" + std::منزل_string(it->ٻيو) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    فراغ گهمو(Ast_ڳنڍ_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filenamep()) iterate_And_ايندڙ_Const_Null(nodep->filenamep());
        putbs(", ");
        إذا (nodep->memp()) iterate_And_ايندڙ_Const_Null(nodep->memp());
        إذا (nodep->lsbp()) {
            putbs(", ");
            iterate_And_ايندڙ_Const_Null(nodep->lsbp());
        }
        إذا (nodep->msbp()) {
            putbs(", ");
            iterate_And_ايندڙ_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    فراغ گهمو(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ايندڙ_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    فراغ گهمو(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_ايندڙ_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    فراغ گهمو(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_ايندڙ_Const_Null(testp->nextp());
            puts("end\n");
            أرجع;
        }
        // Special case when the AstLoopTest is last for output readability
        إذا (Ast_ڳنڍ* lastp = nodep->stmtsp()) {
            طالما (Ast_ڳنڍ* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                لكل (Ast_ڳنڍ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                أرجع;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        iterate_And_ايندڙ_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    فراغ گهمو(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_ايندڙ_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        putfs(nodep, "");
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (ifp->ترجيح_Pragma()) puts("priority ");
            إذا (ifp->uniquePragma()) puts("unique ");
            إذا (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_ايندڙ_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_ايندڙ_Const_Null(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_ايندڙ_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ گهمو(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        إذا (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_ايندڙ_Const_Null(nodep->ticksp());
            إذا (nodep->sentreep()) {
                puts(", ");
                iterate_And_ايندڙ_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    فراغ گهمو(AstSampled* nodep) override {
        إذا (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_ايندڙ_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    فراغ گهمو(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ گهمو(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_ايندڙ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ گهمو(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_ايندڙ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ گهمو(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ گهمو(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ گهمو(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_ايندڙ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ گهمو(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ گهمو(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_ايندڙ_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    فراغ گهمو(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    فراغ گهمو(Ast_پڄاڻي* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ گهمو(Ast_پڄاڻي_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    فراغ گهمو(Ast_متن* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    فراغ گهمو(Ast_متن_بلاڪ* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().خالي_جايون();
        puts(nodep->اڳياڙي());
        emit_Nodes_With_متن(nodep->nodesp(), سچ, nodep->separator());
        puts(nodep->suffix());
    }
    فراغ گهمو(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_متن(nodep->nodesp(), سچ, "");
        puts(");\n");
    }
    فراغ گهمو(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_متن(nodep->nodesp(), سچ, "");
        puts(")");
    }
    فراغ گهمو(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), خطأ, "");
        puts(");\n");
    }
    فراغ گهمو(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), خطأ, "");
        puts(")");
    }

    فراغ گهمو(AstScopeName* nodep) override {}
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_ايندڙ_Const_Null(nodep->resultp());
        puts(");\n");
    }

    فراغ گهمو(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    فراغ گهمو(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    مجازي فراغ emitVerilogFormat(Ast_ڳنڍ* nodep, ثابت string& format, Ast_ڳنڍ* lhsp = خالي_اشارو,
                                   Ast_ڳنڍ* ثابت rhsp = خالي_اشارو, Ast_ڳنڍ* thsp = خالي_اشارو,
                                   Ast_ڳنڍ* fhsp = خالي_اشارو) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        منطقي اندريون_Pct = خطأ;
        putbs("");
        لكل (ثابت حرف c : format) {
            إذا (!اندريون_Pct && c == '%') {
                اندريون_Pct = سچ;
            } وإلا إذا (!اندريون_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } وإلا {  // Format character
                اندريون_Pct = خطأ;
                بدل (c) {
                حالة '%': puts("%"); اكسر;
                حالة 'f': putfs(nodep, ""); اكسر;
                حالة 'k': putbs(""); اكسر;
                حالة 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ايندڙ_Const_Null(lhsp);
                    اكسر;
                }
                حالة 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ايندڙ_Const_Null(rhsp);
                    اكسر;
                }
                حالة 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ايندڙ_Const_Null(thsp);
                    اكسر;
                }
                حالة 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ايندڙ_Const_Null(fhsp);
                    اكسر;
                }
                حالة 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_ايندڙ_Const_Null(nodep->dtypep());
                    اكسر;
                }
                افتراضي: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); اكسر;
                }
            }
        }
    }

    فراغ گهمو(Ast_ڳنڍ_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    فراغ گهمو(Ast_ڳنڍ_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ گهمو(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ گهمو(Ast_Attr_جو* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_ايندڙ_Const_Null(nodep->fromp());
        إذا (nodep->dimp()) {
            putbs(", ");
            iterate_And_ايندڙ_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    فراغ گهمو(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        لكل (ثابت Ast_Cons_Array_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شي)) {
            إذا (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_منزل_Str(itemp->اشاريو()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    فراغ گهمو(AstCond* nodep) override {
        putbs("(");
        iterate_And_ايندڙ_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_ايندڙ_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_ايندڙ_Const_Null(nodep->elsep());
        puts(")");
    }
    فراغ گهمو(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_ايندڙ_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_ايندڙ_Const_Null(nodep->rhsp());
        puts("]");
    }
    فراغ گهمو(AstRange* nodep) override {
        puts("[");
        إذا (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_منزل_Str(nodep->کاٻو_Const()));
            puts(":");
            puts(cvt_منزل_Str(nodep->ساڄو_Const()));
        } وإلا {
            iterate_And_ايندڙ_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_ايندڙ_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    فراغ گهمو(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    فراغ گهمو(AstSel* nodep) override {
        iterate_And_ايندڙ_Const_Null(nodep->fromp());
        صحيح وٿي = 0;
        Ast_ڳنڍ_DType* ثابت dtypep = nodep->fromp()->dtypep();
        إذا (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            وٿي = basicDtypep->lo();
        }
        puts("[");
        إذا (VN_IS(nodep->lsbp(), Const)) {
            إذا (nodep->ويڪر_Const() == 1) {
                puts(cvt_منزل_Str(VN_AS(nodep->lsbp(), Const)->منزل_SInt() + وٿي));
            } وإلا {
                puts(cvt_منزل_Str(VN_AS(nodep->lsbp(), Const)->منزل_SInt() + nodep->ويڪر_Const() + وٿي
                              - 1));
                puts(":");
                puts(cvt_منزل_Str(VN_AS(nodep->lsbp(), Const)->منزل_SInt() + وٿي));
            }
        } وإلا {
            iterate_And_ايندڙ_Const_Null(nodep->lsbp());
            إذا (وٿي != 0) {
                puts(" + ");
                puts(cvt_منزل_Str(وٿي));
            }
            putfs(nodep, "+:");
            puts(cvt_منزل_Str(nodep->ويڪر_Const()));
        }
        puts("]");
    }
    فراغ گهمو(Ast_ٽڪرو_Sel* nodep) override {
        iterate_And_ايندڙ_Const_Null(nodep->fromp());
        puts(cvt_منزل_Str(nodep->declRange()));
    }
    فراغ گهمو(AstThisRef* /*nodep*/) override { puts("this"); }
    فراغ گهمو(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    فراغ گهمو(Ast_ڳنڍ_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        إذا (nodep->userType() == VAssertType::OBSERVED_DEFERRED_فوري) {
            puts("#0 ");
        } وإلا إذا (nodep->userType() == VAssertType::FINAL_DEFERRED_فوري) {
            puts("final ");
        } وإلا إذا (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        إذا (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_ايندڙ_Const_Null(sampledp->exprp());
        } وإلا {
            iterate_And_ايندڙ_Const_Null(nodep->propp());
        }
        إذا (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_ايندڙ_Const_Null(nodep->passsp());
            puts("end\n");
        } وإلا {
            puts(");\n");
        }

        إذا (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_ايندڙ_Const_Null(assertp->failsp());
            puts("end\n");
        } وإلا إذا (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_ايندڙ_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    فراغ گهمو(AstAssocArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[");
            iterateConst(nodep->ڪنجي_DTypep());
            puts("]");
            m_arrayPost = سچ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ گهمو(AstBasicDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, nodep->prettyName());
        إذا (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        إذا (nodep->keyword().isIntNumeric() && !nodep->keyword().is_ٻٽو_انگ_Logic()) أرجع;
        emitPacked();
        إذا (nodep->rangep()) {
            puts(" ");
            iterate_And_ايندڙ_Const_Null(nodep->rangep());
            puts(" ");
        } وإلا إذا (nodep->isRanged()) {
            puts(" [");
            puts(cvt_منزل_Str(nodep->کاٻي()));
            puts(":");
            puts(cvt_منزل_Str(nodep->ساڄي()));
            puts("] ");
        }
    }
    فراغ گهمو(AstConstDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    فراغ گهمو(AstDynArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ گهمو(AstEnumDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_ايندڙ_Const_Null(nodep->itemsp());
        puts("}");
    }
    فراغ گهمو(Ast_Enum_شي_Ref* nodep) override {
        إذا (Ast_ڳنڍ_Module* ثابت classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    فراغ گهمو(Ast_Enum_شي* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        إذا (nodep->nextp()) puts(",");
        puts("\n");
    }
    فراغ گهمو(Ast_ڳنڍ_Array_DType* nodep) override {
        إذا (!m_arrayPost) {
            إذا (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ڌڪو_پٺ(nodep);
            }
            iterateConst(nodep->subDTypep());
        } وإلا {
            إذا (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = خطأ;
                iterate_And_ايندڙ_Const_Null(nodep->rangep());
                m_arrayPost = سچ;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ گهمو(AstIfaceRefDType* nodep) override {
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
    فراغ گهمو(AstRefDType* nodep) override {
        إذا (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } وإلا {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    فراغ گهمو(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::اڳياڙي_Name_Protect(nodep->classp()));
    }
    فراغ گهمو(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ گهمو(AstModport* nodep) override {
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
    فراغ گهمو(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        إذا (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = سچ;
            iterateConst(nodep->varp());
        } وإلا {
            puts(nodep->prettyName());
        }
        إذا (nodep->nextp()) puts(", ");
    }
    فراغ گهمو(Ast_ڳنڍ_UOr_Struct_DType* nodep) override {
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
    فراغ گهمو(AstMemberDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    فراغ گهمو(Ast_قطار_DType* nodep) override {
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
            m_arrayPost = سچ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } وإلا {
            putfs(nodep, nodep->prettyName());
        }
        إذا (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_ايندڙ_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    فراغ گهمو(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_ايندڙ_Const_Null(nodep->argsp());
        puts(")");
    }
    فراغ گهمو(AstArg* nodep) override { iterate_And_ايندڙ_Const_Null(nodep->exprp()); }
    فراغ گهمو(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    فراغ گهمو(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ گهمو(Ast_Print_وقت_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    فراغ گهمو(AstPropSpec* nodep) override {
        إذا (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            لكل (Ast_ڳنڍ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
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
    فراغ گهمو(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    فراغ گهمو(AstAbortOn* nodep) override {
        puts(nodep->قسم().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    فراغ گهمو(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    فراغ گهمو(AstPropAlways* nodep) override {
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
    فراغ گهمو(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = سچ;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } وإلا {
            إذا (nodep->varp()) {
                إذا (nodep->self_اشارو().is_خالي_جايون()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } وإلا {
                    putfs(nodep, nodep->self_اشارو().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } وإلا {
                putfs(nodep, nodep->name());
            }
        }
    }
    فراغ گهمو(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        إذا (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } وإلا {
            puts(nodep->prettyName());
        }
    }
    فراغ گهمو(AstConst* nodep) override { putfs(nodep, nodep->انگ().ascii(m_prefixed, سچ)); }
    فراغ گهمو(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    فراغ گهمو(Ast_چوٽي_Scope* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(AstScope* nodep) override { iterateChildrenConst(nodep); }
    فراغ گهمو(AstVar* nodep) override {
        إذا (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = خطأ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = سچ;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    فراغ گهمو(Ast_سرگرم* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
    }
    فراغ گهمو(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        إذا (nodep->is_چڪر_Delay()) {
            puts("##");
        } وإلا {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = خطأ;
        إذا (Ast_ڳنڍ_اظهار* ثابت fallDelayp = nodep->fallDelay()) {
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
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
    }
    فراغ گهمو(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        إذا (nodep->stmtsp()) {
            puts(" ");
            iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        } وإلا {
            puts(";\n");
        }
    }
    فراغ گهمو(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    فراغ گهمو(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    فراغ گهمو(AstVarScope*) override {}
    فراغ گهمو(AstTraceDecl*) override {}
    فراغ گهمو(AstTraceInc*) override {}
    // NOPs
    فراغ گهمو(AstPragma*) override {}
    فراغ گهمو(AstStmtPragma*) override {}
    فراغ گهمو(AstCell*) override {}  // Handled outside the Visit class
    // Default
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        إذا (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

عام:
    واضح EmitVBaseVisitorConst(منطقي suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = افتراضي;
};

//######################################################################
// Emit to an output file

صنف EmitVFileVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3_ٻاهريون_VFile& m_جو;  // The output file
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_جو.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override { m_جو.puts(str); }
    فراغ putbs(ثابت string& str) override { m_جو.putbs(str); }
    فراغ putfs(Ast_ڳنڍ*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_ڳنڍ*, ثابت string& str) override { putbs(str); }

عام:
    EmitVFileVisitor(Ast_ڳنڍ* nodep, V3_ٻاهريون_VFile& جو, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_جو{جو} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = افتراضي;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

صنف EmitVStreamVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3_ٻاهريون_وهڪرو m_os;  // The output stream formatter
    ثابت منطقي m_tracking;  // Use line tracking
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_os.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    فراغ putbs(ثابت string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    فراغ putfs(Ast_ڳنڍ*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_ڳنڍ*, ثابت string& str) override { putbs(str); }

عام:
    EmitVStreamVisitor(ثابت Ast_ڳنڍ* nodep, std::ostream& os, منطقي tracking, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_ٻاهريون_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ڳنڍ*>(nodep));
    }
    ~EmitVStreamVisitor() override = افتراضي;
};

//######################################################################
// EmitV class functions

فراغ V3EmitV::verilog_For_وڻ(ثابت Ast_ڳنڍ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ خطأ, خطأ}; }
}

فراغ V3EmitV::debug_Verilog_For_وڻ(ثابت Ast_ڳنڍ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ سچ, سچ}; }
}

std::string V3EmitV::debug_Verilog_For_وڻ(ثابت Ast_ڳنڍ* nodep) {
    std::stringstream ss;
    debug_Verilog_For_وڻ(nodep, ss);
    أرجع ss.str();
}

فراغ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (Ast_ڳنڍ_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ڳنڍ_File)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        إذا (vfilep && vfilep->tblockp()) {
            V3_ٻاهريون_VFile جو{vfilep->name()};
            جو.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), جو, خطأ};
        }
    }
}

فراغ V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_ٻاهريون_VFile جو{filename};
    EmitVFileVisitor{v3Global.rootp(), جو, سچ};
}
