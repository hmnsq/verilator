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

جماعت EmitVBaseVisitorConst VL_NOT_FINAL : عوامی VNVisitorConst {
    // STATE - across all visitors
    ثابت منطقی m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_درخت* m_sentreep = خالی_اشاریہ;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقی m_suppressSemi = جھوٹ;  // Non-statement, don't print ;
    منطقی m_suppressVarSemi = جھوٹ;  // Suppress emitting semicolon for AstVars
    منطقی m_suppressSampled = جھوٹ;  // Suppress emitting sampled in assertion properties
    منطقی m_arrayPost = جھوٹ;  // Print array information that goes after identifier (vs after)
    منطقی m_prefixed = سچ;  // Whether constants need to be prefixed
    std::deque<Ast_گرہ_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_بلاک*, سائز_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    مجازی خالی puts(ثابت string& str) = 0;
    مجازی خالی putbs(ثابت string& str) = 0;
    مجازی خالی putfs(Ast_گرہ* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    مجازی خالی putqs(Ast_گرہ* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    مجازی خالی putsNoTracking(ثابت string& str) = 0;
    مجازی خالی putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_برآمد_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    خالی iterateAndCommaConstNull(Ast_گرہ* nodep) {
        برائے (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            اگر (nodep->nextp()) puts(", ");
        }
    }
    خالی emitPacked() {
        برائے (Ast_گرہ_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    خالی emit_Nodes_With_متن(Ast_گرہ* nodesp, منطقی tracking, ثابت std::string& separator) {
        برائے (Ast_گرہ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            اگر (ثابت Ast_متن* ثابت textp = VN_CAST(nodep, متن_2)) {
                اگر (tracking) {
                    puts(textp->متن());
                } ورنہ {
                    putsNoTracking(textp->متن());
                }
            } ورنہ {
                iterateConst(nodep);
            }
            اگر (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override { iterate_And_اگلا_Const_Null(nodep->modulesp()); }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::سابقہ_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    خالی دورہ(Ast_پورٹ* nodep) override {}
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        ثابت std::string name = nodep->name().خالی_خانے() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    خالی دورہ(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_اگلا_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    خالی دورہ(Ast_Gen_Case_شے* nodep) override {
        اگر (nodep->condsp()) {
            iterate_And_اگلا_Const_Null(nodep->condsp());
        } ورنہ {
            putbs("default");
        }
        iterate_And_اگلا_Const_Null(nodep->itemsp());
    }
    خالی دورہ(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = سچ;
            iterate_And_اگلا_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_اگلا_Const_Null(nodep->condp());
            puts(";");
            iterate_And_اگلا_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_اگلا_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_اگلا_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_اگلا_Const_Null(nodep->thensp());
        اگر (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_اگلا_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstBegin* nodep) override {
        اگر (nodep->name() == "") {
            putbs("begin\n");
        } ورنہ {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    خالی دورہ(AstFork* nodep) override {
        اگر (nodep->name() == "") {
            putbs("fork\n");
        } ورنہ {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    خالی دورہ(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(AstAlways* nodep) override {
        اگر (ثابت Ast_تفویض_W* ثابت ap = VN_CAST(nodep->stmtsp(), تفویض_W)) {
            اگر (!ap->nextp()) {
                putfs(nodep, "assign ");
                اگر (Ast_گرہ* ثابت tcp = ap->timingControlp()) {
                    iterate_And_اگلا_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_اگلا_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_اگلا_Const_Null(ap->rhsp());
                اگر (!m_suppressSemi) puts(";\n");
                واپس;
            }
        }
        putfs(nodep, "always ");
        اگر (m_sentreep) {
            iterate_And_اگلا_Const_Null(m_sentreep);
        }  // In active
        ورنہ {
            iterate_And_اگلا_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (VN_IS(nodep, تفویض_Force)) puts("force ");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_اگلا_Const_Null(nodep->rhsp());
        اگر (!m_suppressSemi) puts(";\n");
    }
    خالی دورہ(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        برائے (Ast_گرہ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        اگر (!m_suppressSemi) puts(";\n");
    }
    خالی دورہ(Ast_تفویض_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_اگلا_Const_Null(nodep->rhsp());
        اگر (!m_suppressSemi) puts(";\n");
    }
    خالی دورہ(AstRelease* nodep) override {
        puts("release ");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        اگر (!m_suppressSemi) puts(";\n");
    }
    خالی دورہ(AstBreak*) override {
        putbs("break");
        اگر (!m_suppressSemi) puts(";\n");
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        برائے (Ast_گرہ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            اگر (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        putfs(nodep, "");
        اگر (nodep->کنارہ_Type() != VEdgeType::ET_بدلا) puts(nodep->کنارہ_Type().verilogKwd());
        اگر (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    خالی دورہ(AstCase* nodep) override {
        putfs(nodep, "");
        اگر (nodep->ترجیح_Pragma()) puts("priority ");
        اگر (nodep->uniquePragma()) puts("unique ");
        اگر (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts(")\n");
        اگر (nodep->بھرا_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            اگر (nodep->بھرا_Pragma()) puts(" full_case");
            اگر (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_اگلا_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        اگر (nodep->condsp()) {
            iterate_And_اگلا_Const_Null(nodep->condsp());
        } ورنہ {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstContinue*) override {
        putbs("continue");
        اگر (!m_suppressSemi) puts(";\n");
    }
    خالی دورہ(Ast_گرہ_Cover_Decl*) override {}  // N/A
    خالی دورہ(AstCoverInc*) override {}  // N/A
    خالی دورہ(AstCoverToggle*) override {}  // N/A

    خالی دورہ(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        اگر (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    خالی دورہ(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        اگر (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_اگلا_Const_Null(nodep->optionsp());
            iterate_And_اگلا_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    خالی emitCoverageOption(Ast_گرہ* nodep, منطقی typeOption, VCoverOptionType optType,
                            Ast_گرہ_اظہاریہ* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    خالی دورہ(Ast_Cg_Option_تفویض* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    خالی دورہ(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    خالی دورہ(AstCoverBin* nodep) override {
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
        } ورنہ اگر (nodep->transp()) {
            puts(" = ");
            برائے (Ast_گرہ* setp = nodep->transp(); setp; setp = setp->nextp()) {
                اگر (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } ورنہ اگر (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } ورنہ اگر (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            برائے (Ast_گرہ* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
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
    خالی دورہ(AstCoverWith* nodep) override {
        ثابت منطقی rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        اگر (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        اگر (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    خالی دورہ(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        اگر (!nodep->name().خالی_خانے()) puts("." + nodep->name());
        puts(")");
        اگر (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    خالی دورہ(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        اگر (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    خالی دورہ(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    خالی دورہ(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    خالی دورہ(AstCoverCrossWith* nodep) override {
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
    خالی دورہ(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    خالی دورہ(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        برائے (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
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
            iterate_And_اگلا_Const_Null(nodep->optionsp());
            iterate_And_اگلا_Const_Null(nodep->binsp());
            puts("}\n");
        } ورنہ {
            puts(";\n");
        }
    }
    خالی دورہ(Ast_Cover_Trans_سیٹ* nodep) override {
        puts("(");
        برائے (Ast_گرہ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            اگر (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    خالی دورہ(Ast_Cover_Trans_شے* nodep) override { iterateChildrenConst(nodep); }

    خالی دورہ(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        اگر (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->عدد().مقصد_String());
        } ورنہ {
            puts("string'(");
            iterate_And_اگلا_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    خالی دورہ(Ast_Test_Plus_دلائل* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    خالی دورہ(Ast_قدر_Plus_دلائل* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    خالی دورہ_گرہ_Display(Ast_گرہ* nodep, Ast_گرہ* fileOrStrgp, ثابت string& متن,
                          Ast_گرہ* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(متن);
        برائے (Ast_گرہ* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    خالی دورہ(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    خالی دورہ(AstDisplay* nodep) override {
        دورہ_گرہ_Display(nodep, nodep->filep(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    خالی دورہ(AstElabDisplay* nodep) override {
        دورہ_گرہ_Display(nodep, خالی_اشاریہ, nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    خالی دورہ(AstFScanF* nodep) override {
        دورہ_گرہ_Display(nodep, nodep->filep(), nodep->متن(), nodep->exprsp());
    }
    خالی دورہ(AstSScanF* nodep) override {
        دورہ_گرہ_Display(nodep, nodep->fromp(), nodep->متن(), nodep->exprsp());
    }
    خالی دورہ(AstSFormat* nodep) override {
        دورہ_گرہ_Display(nodep, nodep->lhsp(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    خالی دورہ(Ast_مقصد_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    خالی دورہ(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    خالی دورہ(AstSFormatF* nodep) override {
        دورہ_گرہ_Display(nodep, خالی_اشاریہ, nodep->متن(), nodep->exprsp());
    }
    خالی دورہ(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_اگلا_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_اگلا_Const_Null(nodep->modep());
        puts(");\n");
    }
    خالی دورہ(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_اگلا_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    خالی دورہ(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (nodep->filep()) iterate_And_اگلا_Const_Null(nodep->filep());
        puts(");\n");
    }
    خالی دورہ(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (nodep->filep()) iterate_And_اگلا_Const_Null(nodep->filep());
        puts(");\n");
    }
    خالی دورہ(Ast_Jump_بلاک* nodep) override {
        // Allocate label number
        ثابت سائز_t n = m_labelNumbers.سائز();
        ثابت منطقی new_اندراج = m_labelNumbers.emplace(nodep, n).دوسرا;
        UASSERT_OBJ(new_اندراج, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::مقصد_string(n) + "\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    خالی دورہ(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ثابت خودکار it = m_labelNumbers.ڈھونڈو(nodep->blockp());
        ثابت std::string label
            = it != m_labelNumbers.انتہا() ? "label" + std::مقصد_string(it->دوسرا) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    خالی دورہ(Ast_گرہ_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        اگر (nodep->filenamep()) iterate_And_اگلا_Const_Null(nodep->filenamep());
        putbs(", ");
        اگر (nodep->memp()) iterate_And_اگلا_Const_Null(nodep->memp());
        اگر (nodep->lsbp()) {
            putbs(", ");
            iterate_And_اگلا_Const_Null(nodep->lsbp());
        }
        اگر (nodep->msbp()) {
            putbs(", ");
            iterate_And_اگلا_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    خالی دورہ(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_اگلا_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    خالی دورہ(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_اگلا_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    خالی دورہ(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        اگر (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_اگلا_Const_Null(testp->nextp());
            puts("end\n");
            واپس;
        }
        // Special case when the AstLoopTest is last for output readability
        اگر (Ast_گرہ* lastp = nodep->stmtsp()) {
            جب (Ast_گرہ* ثابت nextp = lastp->nextp()) lastp = nextp;
            اگر (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                برائے (Ast_گرہ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                واپس;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        iterate_And_اگلا_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    خالی دورہ(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_اگلا_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        putfs(nodep, "");
        اگر (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            اگر (ifp->ترجیح_Pragma()) puts("priority ");
            اگر (ifp->uniquePragma()) puts("unique ");
            اگر (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_اگلا_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_اگلا_Const_Null(nodep->thensp());
        اگر (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_اگلا_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    خالی دورہ(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        اگر (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->ticksp());
            اگر (nodep->sentreep()) {
                puts(", ");
                iterate_And_اگلا_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    خالی دورہ(AstSampled* nodep) override {
        اگر (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_اگلا_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    خالی دورہ(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts(")");
    }
    خالی دورہ(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        اگر (nodep->sentreep()) {
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    خالی دورہ(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        اگر (nodep->sentreep()) {
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    خالی دورہ(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts(")");
    }
    خالی دورہ(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts(")");
    }
    خالی دورہ(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        اگر (nodep->sentreep()) {
            puts(", ");
            iterate_And_اگلا_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    خالی دورہ(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_اگلا_Const_Null(nodep->exprp());
        puts(")");
    }
    خالی دورہ(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    خالی دورہ(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    خالی دورہ(Ast_اختتام* nodep) override { putfs(nodep, "$finish;\n"); }
    خالی دورہ(Ast_اختتام_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    خالی دورہ(Ast_متن* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    خالی دورہ(Ast_متن_بلاک* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().خالی_خانے();
        puts(nodep->سابقہ());
        emit_Nodes_With_متن(nodep->nodesp(), سچ, nodep->separator());
        puts(nodep->suffix());
    }
    خالی دورہ(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_متن(nodep->nodesp(), سچ, "");
        puts(");\n");
    }
    خالی دورہ(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_متن(nodep->nodesp(), سچ, "");
        puts(")");
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), جھوٹ, "");
        puts(");\n");
    }
    خالی دورہ(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), جھوٹ, "");
        puts(")");
    }

    خالی دورہ(AstScopeName* nodep) override {}
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_اگلا_Const_Null(nodep->resultp());
        puts(");\n");
    }

    خالی دورہ(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    خالی دورہ(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    مجازی خالی emitVerilogFormat(Ast_گرہ* nodep, ثابت string& format, Ast_گرہ* lhsp = خالی_اشاریہ,
                                   Ast_گرہ* ثابت rhsp = خالی_اشاریہ, Ast_گرہ* thsp = خالی_اشاریہ,
                                   Ast_گرہ* fhsp = خالی_اشاریہ) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        منطقی اندر_Pct = جھوٹ;
        putbs("");
        برائے (ثابت حرف c : format) {
            اگر (!اندر_Pct && c == '%') {
                اندر_Pct = سچ;
            } ورنہ اگر (!اندر_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } ورنہ {  // Format character
                اندر_Pct = جھوٹ;
                انتخاب (c) {
                حالت '%': puts("%"); توڑو;
                حالت 'f': putfs(nodep, ""); توڑو;
                حالت 'k': putbs(""); توڑو;
                حالت 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_اگلا_Const_Null(lhsp);
                    توڑو;
                }
                حالت 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_اگلا_Const_Null(rhsp);
                    توڑو;
                }
                حالت 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_اگلا_Const_Null(thsp);
                    توڑو;
                }
                حالت 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_اگلا_Const_Null(fhsp);
                    توڑو;
                }
                حالت 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_اگلا_Const_Null(nodep->dtypep());
                    توڑو;
                }
                طےشدہ: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); توڑو;
                }
            }
        }
    }

    خالی دورہ(Ast_گرہ_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    خالی دورہ(Ast_گرہ_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    خالی دورہ(Ast_گرہ_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    خالی دورہ(Ast_گرہ_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    خالی دورہ(Ast_گرہ_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    خالی دورہ(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    خالی دورہ(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    خالی دورہ(Ast_Attr_کا* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_اگلا_Const_Null(nodep->fromp());
        اگر (nodep->dimp()) {
            putbs(", ");
            iterate_And_اگلا_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    خالی دورہ(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        برائے (ثابت Ast_Cons_Array_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شے)) {
            اگر (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_مقصد_Str(itemp->اشاریہ()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    خالی دورہ(AstCond* nodep) override {
        putbs("(");
        iterate_And_اگلا_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_اگلا_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_اگلا_Const_Null(nodep->elsep());
        puts(")");
    }
    خالی دورہ(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_اگلا_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_اگلا_Const_Null(nodep->rhsp());
        puts("]");
    }
    خالی دورہ(AstRange* nodep) override {
        puts("[");
        اگر (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_مقصد_Str(nodep->بایاں_Const()));
            puts(":");
            puts(cvt_مقصد_Str(nodep->دایاں_Const()));
        } ورنہ {
            iterate_And_اگلا_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_اگلا_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    خالی دورہ(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    خالی دورہ(AstSel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        صحیح انحراف = 0;
        Ast_گرہ_DType* ثابت dtypep = nodep->fromp()->dtypep();
        اگر (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            انحراف = basicDtypep->lo();
        }
        puts("[");
        اگر (VN_IS(nodep->lsbp(), Const)) {
            اگر (nodep->چوڑائی_Const() == 1) {
                puts(cvt_مقصد_Str(VN_AS(nodep->lsbp(), Const)->مقصد_SInt() + انحراف));
            } ورنہ {
                puts(cvt_مقصد_Str(VN_AS(nodep->lsbp(), Const)->مقصد_SInt() + nodep->چوڑائی_Const() + انحراف
                              - 1));
                puts(":");
                puts(cvt_مقصد_Str(VN_AS(nodep->lsbp(), Const)->مقصد_SInt() + انحراف));
            }
        } ورنہ {
            iterate_And_اگلا_Const_Null(nodep->lsbp());
            اگر (انحراف != 0) {
                puts(" + ");
                puts(cvt_مقصد_Str(انحراف));
            }
            putfs(nodep, "+:");
            puts(cvt_مقصد_Str(nodep->چوڑائی_Const()));
        }
        puts("]");
    }
    خالی دورہ(Ast_قاش_Sel* nodep) override {
        iterate_And_اگلا_Const_Null(nodep->fromp());
        puts(cvt_مقصد_Str(nodep->declRange()));
    }
    خالی دورہ(AstThisRef* /*nodep*/) override { puts("this"); }
    خالی دورہ(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    خالی دورہ(Ast_گرہ_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        اگر (nodep->userType() == VAssertType::OBSERVED_DEFERRED_فوری) {
            puts("#0 ");
        } ورنہ اگر (nodep->userType() == VAssertType::FINAL_DEFERRED_فوری) {
            puts("final ");
        } ورنہ اگر (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        اگر (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_اگلا_Const_Null(sampledp->exprp());
        } ورنہ {
            iterate_And_اگلا_Const_Null(nodep->propp());
        }
        اگر (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_اگلا_Const_Null(nodep->passsp());
            puts("end\n");
        } ورنہ {
            puts(");\n");
        }

        اگر (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_اگلا_Const_Null(assertp->failsp());
            puts("end\n");
        } ورنہ اگر (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_اگلا_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    خالی دورہ(AstAssocArrayDType* nodep) override {
        اگر (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ورنہ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = جھوٹ;
            puts("[");
            iterateConst(nodep->کلید_DTypep());
            puts("]");
            m_arrayPost = سچ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    خالی دورہ(AstBasicDType* nodep) override {
        اگر (m_arrayPost) واپس;
        putfs(nodep, nodep->prettyName());
        اگر (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        اگر (nodep->keyword().isIntNumeric() && !nodep->keyword().is_ثنائی_ہندسہ_Logic()) واپس;
        emitPacked();
        اگر (nodep->rangep()) {
            puts(" ");
            iterate_And_اگلا_Const_Null(nodep->rangep());
            puts(" ");
        } ورنہ اگر (nodep->isRanged()) {
            puts(" [");
            puts(cvt_مقصد_Str(nodep->بائیں()));
            puts(":");
            puts(cvt_مقصد_Str(nodep->دائیں()));
            puts("] ");
        }
    }
    خالی دورہ(AstConstDType* nodep) override {
        اگر (m_arrayPost) واپس;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    خالی دورہ(AstDynArrayDType* nodep) override {
        اگر (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ورنہ {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    خالی دورہ(AstEnumDType* nodep) override {
        اگر (m_arrayPost) واپس;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_اگلا_Const_Null(nodep->itemsp());
        puts("}");
    }
    خالی دورہ(Ast_Enum_شے_Ref* nodep) override {
        اگر (Ast_گرہ_Module* ثابت classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    خالی دورہ(Ast_Enum_شے* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        اگر (nodep->nextp()) puts(",");
        puts("\n");
    }
    خالی دورہ(Ast_گرہ_Array_DType* nodep) override {
        اگر (!m_arrayPost) {
            اگر (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ڈالو_عقب(nodep);
            }
            iterateConst(nodep->subDTypep());
        } ورنہ {
            اگر (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = جھوٹ;
                iterate_And_اگلا_Const_Null(nodep->rangep());
                m_arrayPost = سچ;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    خالی دورہ(AstIfaceRefDType* nodep) override {
        اگر (m_arrayPost) {
            puts(" (");
            اگر (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } ورنہ {
                puts("????");
            }
            puts(")");
            واپس;
        }
        puts(nodep->ifaceName());
    }
    خالی دورہ(AstRefDType* nodep) override {
        اگر (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } ورنہ {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    خالی دورہ(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::سابقہ_Name_Protect(nodep->classp()));
    }
    خالی دورہ(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    خالی دورہ(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        اگر (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } ورنہ {
            puts("????");
        }
        puts(");\n");
    }
    خالی دورہ(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        اگر (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = سچ;
            iterateConst(nodep->varp());
        } ورنہ {
            puts(nodep->prettyName());
        }
        اگر (nodep->nextp()) puts(", ");
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        اگر (m_arrayPost) واپس;
        puts(nodep->verilogKwd() + " ");
        اگر (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            برائے (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    خالی دورہ(AstMemberDType* nodep) override {
        اگر (m_arrayPost) واپس;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    خالی دورہ(Ast_قطار_DType* nodep) override {
        اگر (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ورنہ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = جھوٹ;
            puts("[$");
            اگر (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = سچ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        اگر (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } ورنہ {
            putfs(nodep, nodep->prettyName());
        }
        اگر (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_اگلا_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    خالی دورہ(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_اگلا_Const_Null(nodep->argsp());
        puts(")");
    }
    خالی دورہ(AstArg* nodep) override { iterate_And_اگلا_Const_Null(nodep->exprp()); }
    خالی دورہ(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    خالی دورہ(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    خالی دورہ(Ast_Print_وقت_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    خالی دورہ(AstPropSpec* nodep) override {
        اگر (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            برائے (Ast_گرہ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
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
    خالی دورہ(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    خالی دورہ(AstAbortOn* nodep) override {
        puts(nodep->قسم().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    خالی دورہ(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    خالی دورہ(AstPropAlways* nodep) override {
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
    خالی دورہ(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = سچ;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    خالی دورہ(AstVarRef* nodep) override {
        اگر (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } ورنہ {
            اگر (nodep->varp()) {
                اگر (nodep->self_اشارہ_گر().is_خالی_خانے()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } ورنہ {
                    putfs(nodep, nodep->self_اشارہ_گر().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } ورنہ {
                putfs(nodep, nodep->name());
            }
        }
    }
    خالی دورہ(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        اگر (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } ورنہ {
            puts(nodep->prettyName());
        }
    }
    خالی دورہ(AstConst* nodep) override { putfs(nodep, nodep->عدد().ascii(m_prefixed, سچ)); }
    خالی دورہ(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    خالی دورہ(Ast_چوٹی_Scope* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(AstScope* nodep) override { iterateChildrenConst(nodep); }
    خالی دورہ(AstVar* nodep) override {
        اگر (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = جھوٹ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = سچ;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    خالی دورہ(Ast_فعال* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }
    خالی دورہ(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        اگر (nodep->is_چکر_Delay()) {
            puts("##");
        } ورنہ {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = جھوٹ;
        اگر (Ast_گرہ_اظہاریہ* ثابت fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } ورنہ {
            iterateConst(nodep->lhsp());
        }
        اگر (!m_suppressSemi) {
            puts(";\n");
        } ورنہ {
            puts(" ");
        }
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }
    خالی دورہ(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        اگر (nodep->stmtsp()) {
            puts(" ");
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
        } ورنہ {
            puts(";\n");
        }
    }
    خالی دورہ(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    خالی دورہ(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    خالی دورہ(AstVarScope*) override {}
    خالی دورہ(AstTraceDecl*) override {}
    خالی دورہ(AstTraceInc*) override {}
    // NOPs
    خالی دورہ(AstPragma*) override {}
    خالی دورہ(AstStmtPragma*) override {}
    خالی دورہ(AstCell*) override {}  // Handled outside the Visit class
    // Default
    خالی دورہ(Ast_گرہ* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        اگر (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

عوامی:
    صریح EmitVBaseVisitorConst(منطقی suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = طےشدہ;
};

//######################################################################
// Emit to an output file

جماعت EmitVFileVisitor final : عوامی EmitVBaseVisitorConst {
    // STATE
    V3_برآمد_VFile& m_کا;  // The output file
    // METHODS
    خالی putsNoTracking(ثابت string& str) override { m_کا.putsNoTracking(str); }
    خالی puts(ثابت string& str) override { m_کا.puts(str); }
    خالی putbs(ثابت string& str) override { m_کا.putbs(str); }
    خالی putfs(Ast_گرہ*, ثابت string& str) override { putbs(str); }
    خالی putqs(Ast_گرہ*, ثابت string& str) override { putbs(str); }

عوامی:
    EmitVFileVisitor(Ast_گرہ* nodep, V3_برآمد_VFile& کا, منطقی suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_کا{کا} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = طےشدہ;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

جماعت EmitVStreamVisitor final : عوامی EmitVBaseVisitorConst {
    // STATE
    V3_برآمد_دھارا m_os;  // The output stream formatter
    ثابت منطقی m_tracking;  // Use line tracking
    // METHODS
    خالی putsNoTracking(ثابت string& str) override { m_os.putsNoTracking(str); }
    خالی puts(ثابت string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    خالی putbs(ثابت string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    خالی putfs(Ast_گرہ*, ثابت string& str) override { putbs(str); }
    خالی putqs(Ast_گرہ*, ثابت string& str) override { putbs(str); }

عوامی:
    EmitVStreamVisitor(ثابت Ast_گرہ* nodep, std::ostream& os, منطقی tracking, منطقی suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_برآمد_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_گرہ*>(nodep));
    }
    ~EmitVStreamVisitor() override = طےشدہ;
};

//######################################################################
// EmitV class functions

خالی V3EmitV::verilog_For_درخت(ثابت Ast_گرہ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ جھوٹ, جھوٹ}; }
}

خالی V3EmitV::debug_Verilog_For_درخت(ثابت Ast_گرہ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ سچ, سچ}; }
}

std::string V3EmitV::debug_Verilog_For_درخت(ثابت Ast_گرہ* nodep) {
    std::stringstream ss;
    debug_Verilog_For_درخت(nodep, ss);
    واپس ss.str();
}

خالی V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    برائے (Ast_گرہ_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), گرہ_File)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        اگر (vfilep && vfilep->tblockp()) {
            V3_برآمد_VFile کا{vfilep->name()};
            کا.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), کا, جھوٹ};
        }
    }
}

خالی V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_برآمد_VFile کا{filename};
    EmitVFileVisitor{v3Global.rootp(), کا, سچ};
}
