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
    Ast_Sen_درخت* m_sentreep = اشاره_تهی;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقي m_suppressSemi = خطأ;  // Non-statement, don't print ;
    منطقي m_suppressVarSemi = خطأ;  // Suppress emitting semicolon for AstVars
    منطقي m_suppressSampled = خطأ;  // Suppress emitting sampled in assertion properties
    منطقي m_arrayPost = خطأ;  // Print array information that goes after identifier (vs after)
    منطقي m_prefixed = درست;  // Whether constants need to be prefixed
    std::deque<Ast_گره_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_بلوک*, اندازه_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    مجازی فراغ puts(ثابت string& str) = 0;
    مجازی فراغ putbs(ثابت string& str) = 0;
    مجازی فراغ putfs(Ast_گره* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    مجازی فراغ putqs(Ast_گره* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    مجازی فراغ putsNoTracking(ثابت string& str) = 0;
    مجازی فراغ putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_بیرون_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    فراغ iterateAndCommaConstNull(Ast_گره* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            إذا (nodep->nextp()) puts(", ");
        }
    }
    فراغ emitPacked() {
        لكل (Ast_گره_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    فراغ emit_Nodes_With_متن(Ast_گره* nodesp, منطقي tracking, ثابت std::string& separator) {
        لكل (Ast_گره* nodep = nodesp; nodep; nodep = nodep->nextp()) {
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
    فراغ بازدید(AstNetlist* nodep) override { iterate_And_بعدی_Const_Null(nodep->modulesp()); }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::پیشوند_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    فراغ بازدید(Ast_پورت* nodep) override {}
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    فراغ بازدید(Ast_Gen_بلوک* nodep) override {
        ثابت std::string name = nodep->name().خالی() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    فراغ بازدید(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_بعدی_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ بازدید(Ast_Gen_Case_قلم* nodep) override {
        إذا (nodep->condsp()) {
            iterate_And_بعدی_Const_Null(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        iterate_And_بعدی_Const_Null(nodep->itemsp());
    }
    فراغ بازدید(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = درست;
            iterate_And_بعدی_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_بعدی_Const_Null(nodep->condp());
            puts(";");
            iterate_And_بعدی_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_بعدی_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_بعدی_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_بعدی_Const_Null(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_بعدی_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstBegin* nodep) override {
        إذا (nodep->name() == "") {
            putbs("begin\n");
        } وإلا {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    فراغ بازدید(AstFork* nodep) override {
        إذا (nodep->name() == "") {
            putbs("fork\n");
        } وإلا {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    فراغ بازدید(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ بازدید(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    فراغ بازدید(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ بازدید(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    فراغ بازدید(AstAlways* nodep) override {
        إذا (ثابت Ast_تخصیص_بده_W* ثابت ap = VN_CAST(nodep->stmtsp(), تخصیص_بده_W)) {
            إذا (!ap->nextp()) {
                putfs(nodep, "assign ");
                إذا (Ast_گره* ثابت tcp = ap->timingControlp()) {
                    iterate_And_بعدی_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_بعدی_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_بعدی_Const_Null(ap->rhsp());
                إذا (!m_suppressSemi) puts(";\n");
                أرجع;
            }
        }
        putfs(nodep, "always ");
        إذا (m_sentreep) {
            iterate_And_بعدی_Const_Null(m_sentreep);
        }  // In active
        وإلا {
            iterate_And_بعدی_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        إذا (VN_IS(nodep, تخصیص_بده_Force)) puts("force ");
        iterate_And_بعدی_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_بعدی_Const_Null(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ بازدید(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        لكل (Ast_گره* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ بازدید(Ast_تخصیص_بده_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_بعدی_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_بعدی_Const_Null(nodep->rhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ بازدید(AstRelease* nodep) override {
        puts("release ");
        iterate_And_بعدی_Const_Null(nodep->lhsp());
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ بازدید(AstBreak*) override {
        putbs("break");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ بازدید(Ast_Sen_درخت* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        لكل (Ast_گره* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            إذا (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    فراغ بازدید(Ast_Sen_قلم* nodep) override {
        putfs(nodep, "");
        إذا (nodep->یال_Type() != VEdgeType::ET_تغییریافته) puts(nodep->یال_Type().verilogKwd());
        إذا (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    فراغ بازدید(AstCase* nodep) override {
        putfs(nodep, "");
        إذا (nodep->اولویت_Pragma()) puts("priority ");
        إذا (nodep->uniquePragma()) puts("unique ");
        إذا (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        puts(")\n");
        إذا (nodep->پر_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            إذا (nodep->پر_Pragma()) puts(" full_case");
            إذا (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_بعدی_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ بازدید(Ast_Case_قلم* nodep) override {
        إذا (nodep->condsp()) {
            iterate_And_بعدی_Const_Null(nodep->condsp());
        } وإلا {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstContinue*) override {
        putbs("continue");
        إذا (!m_suppressSemi) puts(";\n");
    }
    فراغ بازدید(Ast_گره_Cover_Decl*) override {}  // N/A
    فراغ بازدید(AstCoverInc*) override {}  // N/A
    فراغ بازدید(AstCoverToggle*) override {}  // N/A

    فراغ بازدید(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        إذا (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    فراغ بازدید(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        إذا (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_بعدی_Const_Null(nodep->optionsp());
            iterate_And_بعدی_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    فراغ emitCoverageOption(Ast_گره* nodep, منطقي typeOption, VCoverOptionType optType,
                            Ast_گره_عبارت* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    فراغ بازدید(Ast_Cg_Option_تخصیص_بده* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ بازدید(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    فراغ بازدید(AstCoverBin* nodep) override {
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
            لكل (Ast_گره* setp = nodep->transp(); setp; setp = setp->nextp()) {
                إذا (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } وإلا إذا (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } وإلا إذا (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            لكل (Ast_گره* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
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
    فراغ بازدید(AstCoverWith* nodep) override {
        ثابت منطقي rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        إذا (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        إذا (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    فراغ بازدید(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        إذا (!nodep->name().خالی()) puts("." + nodep->name());
        puts(")");
        إذا (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    فراغ بازدید(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        إذا (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    فراغ بازدید(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ بازدید(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    فراغ بازدید(AstCoverCrossWith* nodep) override {
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
    فراغ بازدید(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    فراغ بازدید(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        لكل (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
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
            iterate_And_بعدی_Const_Null(nodep->optionsp());
            iterate_And_بعدی_Const_Null(nodep->binsp());
            puts("}\n");
        } وإلا {
            puts(";\n");
        }
    }
    فراغ بازدید(Ast_Cover_Trans_مجموعه* nodep) override {
        puts("(");
        لكل (Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    فراغ بازدید(Ast_Cover_Trans_قلم* nodep) override { iterateChildrenConst(nodep); }

    فراغ بازدید(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        إذا (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->رقم().مقصد_String());
        } وإلا {
            puts("string'(");
            iterate_And_بعدی_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    فراغ بازدید(Ast_Test_Plus_آرگومانها* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    فراغ بازدید(Ast_مقدار_Plus_آرگومانها* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    فراغ بازدید_گره_Display(Ast_گره* nodep, Ast_گره* fileOrStrgp, ثابت string& متن,
                          Ast_گره* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(متن);
        لكل (Ast_گره* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    فراغ بازدید(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    فراغ بازدید(AstDisplay* nodep) override {
        بازدید_گره_Display(nodep, nodep->filep(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ بازدید(AstElabDisplay* nodep) override {
        بازدید_گره_Display(nodep, اشاره_تهی, nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ بازدید(AstFScanF* nodep) override {
        بازدید_گره_Display(nodep, nodep->filep(), nodep->متن(), nodep->exprsp());
    }
    فراغ بازدید(AstSScanF* nodep) override {
        بازدید_گره_Display(nodep, nodep->fromp(), nodep->متن(), nodep->exprsp());
    }
    فراغ بازدید(AstSFormat* nodep) override {
        بازدید_گره_Display(nodep, nodep->lhsp(), nodep->fmtp()->متن(), nodep->fmtp()->exprsp());
    }
    فراغ بازدید(Ast_مقصد_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ بازدید(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    فراغ بازدید(AstSFormatF* nodep) override {
        بازدید_گره_Display(nodep, اشاره_تهی, nodep->متن(), nodep->exprsp());
    }
    فراغ بازدید(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_بعدی_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_بعدی_Const_Null(nodep->modep());
        puts(");\n");
    }
    فراغ بازدید(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_بعدی_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    فراغ بازدید(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterate_And_بعدی_Const_Null(nodep->filep());
        puts(");\n");
    }
    فراغ بازدید(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filep()) iterate_And_بعدی_Const_Null(nodep->filep());
        puts(");\n");
    }
    فراغ بازدید(Ast_Jump_بلوک* nodep) override {
        // Allocate label number
        ثابت اندازه_t n = m_labelNumbers.اندازه();
        ثابت منطقي new_مدخل = m_labelNumbers.emplace(nodep, n).دومی;
        UASSERT_OBJ(new_مدخل, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::مقصد_string(n) + "\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    فراغ بازدید(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ثابت خودکار it = m_labelNumbers.پیدا_کن(nodep->blockp());
        ثابت std::string label
            = it != m_labelNumbers.انتها() ? "label" + std::مقصد_string(it->دومی) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    فراغ بازدید(Ast_گره_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        إذا (nodep->filenamep()) iterate_And_بعدی_Const_Null(nodep->filenamep());
        putbs(", ");
        إذا (nodep->memp()) iterate_And_بعدی_Const_Null(nodep->memp());
        إذا (nodep->lsbp()) {
            putbs(", ");
            iterate_And_بعدی_Const_Null(nodep->lsbp());
        }
        إذا (nodep->msbp()) {
            putbs(", ");
            iterate_And_بعدی_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    فراغ بازدید(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_بعدی_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    فراغ بازدید(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_بعدی_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    فراغ بازدید(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        إذا (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_بعدی_Const_Null(testp->nextp());
            puts("end\n");
            أرجع;
        }
        // Special case when the AstLoopTest is last for output readability
        إذا (Ast_گره* lastp = nodep->stmtsp()) {
            طالما (Ast_گره* ثابت nextp = lastp->nextp()) lastp = nextp;
            إذا (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                لكل (Ast_گره* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                أرجع;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        iterate_And_بعدی_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    فراغ بازدید(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_بعدی_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        putfs(nodep, "");
        إذا (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (ifp->اولویت_Pragma()) puts("priority ");
            إذا (ifp->uniquePragma()) puts("unique ");
            إذا (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_بعدی_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_بعدی_Const_Null(nodep->thensp());
        إذا (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_بعدی_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ بازدید(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        إذا (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_بعدی_Const_Null(nodep->ticksp());
            إذا (nodep->sentreep()) {
                puts(", ");
                iterate_And_بعدی_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    فراغ بازدید(AstSampled* nodep) override {
        إذا (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_بعدی_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    فراغ بازدید(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ بازدید(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_بعدی_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ بازدید(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_بعدی_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ بازدید(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ بازدید(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ بازدید(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        إذا (nodep->sentreep()) {
            puts(", ");
            iterate_And_بعدی_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    فراغ بازدید(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_بعدی_Const_Null(nodep->exprp());
        puts(")");
    }
    فراغ بازدید(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_بعدی_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    فراغ بازدید(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    فراغ بازدید(Ast_پایان* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ بازدید(Ast_پایان_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    فراغ بازدید(Ast_متن* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    فراغ بازدید(Ast_متن_بلوک* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().خالی();
        puts(nodep->پیشوند());
        emit_Nodes_With_متن(nodep->nodesp(), درست, nodep->separator());
        puts(nodep->suffix());
    }
    فراغ بازدید(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_متن(nodep->nodesp(), درست, "");
        puts(");\n");
    }
    فراغ بازدید(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_متن(nodep->nodesp(), درست, "");
        puts(")");
    }
    فراغ بازدید(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), خطأ, "");
        puts(");\n");
    }
    فراغ بازدید(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_متن(nodep->nodesp(), خطأ, "");
        puts(")");
    }

    فراغ بازدید(AstScopeName* nodep) override {}
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_بعدی_Const_Null(nodep->resultp());
        puts(");\n");
    }

    فراغ بازدید(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    فراغ بازدید(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    مجازی فراغ emitVerilogFormat(Ast_گره* nodep, ثابت string& format, Ast_گره* lhsp = اشاره_تهی,
                                   Ast_گره* ثابت rhsp = اشاره_تهی, Ast_گره* thsp = اشاره_تهی,
                                   Ast_گره* fhsp = اشاره_تهی) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        منطقي درون_Pct = خطأ;
        putbs("");
        لكل (ثابت حرف c : format) {
            إذا (!درون_Pct && c == '%') {
                درون_Pct = درست;
            } وإلا إذا (!درون_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } وإلا {  // Format character
                درون_Pct = خطأ;
                بدل (c) {
                حالة '%': puts("%"); اكسر;
                حالة 'f': putfs(nodep, ""); اكسر;
                حالة 'k': putbs(""); اكسر;
                حالة 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بعدی_Const_Null(lhsp);
                    اكسر;
                }
                حالة 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بعدی_Const_Null(rhsp);
                    اكسر;
                }
                حالة 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بعدی_Const_Null(thsp);
                    اكسر;
                }
                حالة 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_بعدی_Const_Null(fhsp);
                    اكسر;
                }
                حالة 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_بعدی_Const_Null(nodep->dtypep());
                    اكسر;
                }
                افتراضي: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); اكسر;
                }
            }
        }
    }

    فراغ بازدید(Ast_گره_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    فراغ بازدید(Ast_گره_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    فراغ بازدید(Ast_گره_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    فراغ بازدید(Ast_گره_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    فراغ بازدید(Ast_گره_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ بازدید(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    فراغ بازدید(Ast_Attr_از* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_بعدی_Const_Null(nodep->fromp());
        إذا (nodep->dimp()) {
            putbs(", ");
            iterate_And_بعدی_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    فراغ بازدید(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        لكل (ثابت Ast_Cons_Array_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_قلم)) {
            إذا (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_مقصد_Str(itemp->شاخص()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    فراغ بازدید(AstCond* nodep) override {
        putbs("(");
        iterate_And_بعدی_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_بعدی_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_بعدی_Const_Null(nodep->elsep());
        puts(")");
    }
    فراغ بازدید(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_بعدی_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_بعدی_Const_Null(nodep->rhsp());
        puts("]");
    }
    فراغ بازدید(AstRange* nodep) override {
        puts("[");
        إذا (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_مقصد_Str(nodep->چپ_Const()));
            puts(":");
            puts(cvt_مقصد_Str(nodep->راست_Const()));
        } وإلا {
            iterate_And_بعدی_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_بعدی_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    فراغ بازدید(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    فراغ بازدید(AstSel* nodep) override {
        iterate_And_بعدی_Const_Null(nodep->fromp());
        صحيح جابه_جایی = 0;
        Ast_گره_DType* ثابت dtypep = nodep->fromp()->dtypep();
        إذا (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            جابه_جایی = basicDtypep->lo();
        }
        puts("[");
        إذا (VN_IS(nodep->lsbp(), Const)) {
            إذا (nodep->عرض_Const() == 1) {
                puts(cvt_مقصد_Str(VN_AS(nodep->lsbp(), Const)->مقصد_SInt() + جابه_جایی));
            } وإلا {
                puts(cvt_مقصد_Str(VN_AS(nodep->lsbp(), Const)->مقصد_SInt() + nodep->عرض_Const() + جابه_جایی
                              - 1));
                puts(":");
                puts(cvt_مقصد_Str(VN_AS(nodep->lsbp(), Const)->مقصد_SInt() + جابه_جایی));
            }
        } وإلا {
            iterate_And_بعدی_Const_Null(nodep->lsbp());
            إذا (جابه_جایی != 0) {
                puts(" + ");
                puts(cvt_مقصد_Str(جابه_جایی));
            }
            putfs(nodep, "+:");
            puts(cvt_مقصد_Str(nodep->عرض_Const()));
        }
        puts("]");
    }
    فراغ بازدید(Ast_برش_Sel* nodep) override {
        iterate_And_بعدی_Const_Null(nodep->fromp());
        puts(cvt_مقصد_Str(nodep->declRange()));
    }
    فراغ بازدید(AstThisRef* /*nodep*/) override { puts("this"); }
    فراغ بازدید(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    فراغ بازدید(Ast_گره_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        إذا (nodep->userType() == VAssertType::OBSERVED_DEFERRED_فوری) {
            puts("#0 ");
        } وإلا إذا (nodep->userType() == VAssertType::FINAL_DEFERRED_فوری) {
            puts("final ");
        } وإلا إذا (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        إذا (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_بعدی_Const_Null(sampledp->exprp());
        } وإلا {
            iterate_And_بعدی_Const_Null(nodep->propp());
        }
        إذا (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_بعدی_Const_Null(nodep->passsp());
            puts("end\n");
        } وإلا {
            puts(");\n");
        }

        إذا (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_بعدی_Const_Null(assertp->failsp());
            puts("end\n");
        } وإلا إذا (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_بعدی_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    فراغ بازدید(AstAssocArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[");
            iterateConst(nodep->کلید_DTypep());
            puts("]");
            m_arrayPost = درست;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ بازدید(AstBasicDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, nodep->prettyName());
        إذا (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        إذا (nodep->keyword().isIntNumeric() && !nodep->keyword().is_بیت_Logic()) أرجع;
        emitPacked();
        إذا (nodep->rangep()) {
            puts(" ");
            iterate_And_بعدی_Const_Null(nodep->rangep());
            puts(" ");
        } وإلا إذا (nodep->isRanged()) {
            puts(" [");
            puts(cvt_مقصد_Str(nodep->چپ()));
            puts(":");
            puts(cvt_مقصد_Str(nodep->راست()));
            puts("] ");
        }
    }
    فراغ بازدید(AstConstDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    فراغ بازدید(AstDynArrayDType* nodep) override {
        إذا (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ بازدید(AstEnumDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_بعدی_Const_Null(nodep->itemsp());
        puts("}");
    }
    فراغ بازدید(Ast_Enum_قلم_Ref* nodep) override {
        إذا (Ast_گره_Module* ثابت classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    فراغ بازدید(Ast_Enum_قلم* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        إذا (nodep->nextp()) puts(",");
        puts("\n");
    }
    فراغ بازدید(Ast_گره_Array_DType* nodep) override {
        إذا (!m_arrayPost) {
            إذا (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.افزودن_پشت(nodep);
            }
            iterateConst(nodep->subDTypep());
        } وإلا {
            إذا (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = خطأ;
                iterate_And_بعدی_Const_Null(nodep->rangep());
                m_arrayPost = درست;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ بازدید(AstIfaceRefDType* nodep) override {
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
    فراغ بازدید(AstRefDType* nodep) override {
        إذا (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } وإلا {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    فراغ بازدید(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::پیشوند_Name_Protect(nodep->classp()));
    }
    فراغ بازدید(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ بازدید(AstModport* nodep) override {
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
    فراغ بازدید(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        إذا (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = درست;
            iterateConst(nodep->varp());
        } وإلا {
            puts(nodep->prettyName());
        }
        إذا (nodep->nextp()) puts(", ");
    }
    فراغ بازدید(Ast_گره_UOr_Struct_DType* nodep) override {
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
    فراغ بازدید(AstMemberDType* nodep) override {
        إذا (m_arrayPost) أرجع;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    فراغ بازدید(Ast_صف_DType* nodep) override {
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
            m_arrayPost = درست;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } وإلا {
            putfs(nodep, nodep->prettyName());
        }
        إذا (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_بعدی_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    فراغ بازدید(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_بعدی_Const_Null(nodep->argsp());
        puts(")");
    }
    فراغ بازدید(AstArg* nodep) override { iterate_And_بعدی_Const_Null(nodep->exprp()); }
    فراغ بازدید(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    فراغ بازدید(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    فراغ بازدید(Ast_Print_زمان_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    فراغ بازدید(AstPropSpec* nodep) override {
        إذا (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            لكل (Ast_گره* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
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
    فراغ بازدید(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    فراغ بازدید(AstAbortOn* nodep) override {
        puts(nodep->نوع().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    فراغ بازدید(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    فراغ بازدید(AstPropAlways* nodep) override {
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
    فراغ بازدید(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = درست;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    فراغ بازدید(AstVarRef* nodep) override {
        إذا (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } وإلا {
            إذا (nodep->varp()) {
                إذا (nodep->self_اشاره_گر().is_خالی()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } وإلا {
                    putfs(nodep, nodep->self_اشاره_گر().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } وإلا {
                putfs(nodep, nodep->name());
            }
        }
    }
    فراغ بازدید(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        إذا (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } وإلا {
            puts(nodep->prettyName());
        }
    }
    فراغ بازدید(AstConst* nodep) override { putfs(nodep, nodep->رقم().ascii(m_prefixed, درست)); }
    فراغ بازدید(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    فراغ بازدید(Ast_بالاترین_Scope* nodep) override { iterateChildrenConst(nodep); }
    فراغ بازدید(AstScope* nodep) override { iterateChildrenConst(nodep); }
    فراغ بازدید(AstVar* nodep) override {
        إذا (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = خطأ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = درست;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    فراغ بازدید(Ast_فعال* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
    }
    فراغ بازدید(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        إذا (nodep->is_چرخه_Delay()) {
            puts("##");
        } وإلا {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = خطأ;
        إذا (Ast_گره_عبارت* ثابت fallDelayp = nodep->fallDelay()) {
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
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
    }
    فراغ بازدید(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        إذا (nodep->stmtsp()) {
            puts(" ");
            iterate_And_بعدی_Const_Null(nodep->stmtsp());
        } وإلا {
            puts(";\n");
        }
    }
    فراغ بازدید(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    فراغ بازدید(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    فراغ بازدید(AstVarScope*) override {}
    فراغ بازدید(AstTraceDecl*) override {}
    فراغ بازدید(AstTraceInc*) override {}
    // NOPs
    فراغ بازدید(AstPragma*) override {}
    فراغ بازدید(AstStmtPragma*) override {}
    فراغ بازدید(AstCell*) override {}  // Handled outside the Visit class
    // Default
    فراغ بازدید(Ast_گره* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        إذا (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

عام:
    صریح EmitVBaseVisitorConst(منطقي suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = افتراضي;
};

//######################################################################
// Emit to an output file

صنف EmitVFileVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3_بیرون_VFile& m_از;  // The output file
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_از.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override { m_از.puts(str); }
    فراغ putbs(ثابت string& str) override { m_از.putbs(str); }
    فراغ putfs(Ast_گره*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_گره*, ثابت string& str) override { putbs(str); }

عام:
    EmitVFileVisitor(Ast_گره* nodep, V3_بیرون_VFile& از, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_از{از} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = افتراضي;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

صنف EmitVStreamVisitor final : عام EmitVBaseVisitorConst {
    // STATE
    V3_بیرون_جریان m_os;  // The output stream formatter
    ثابت منطقي m_tracking;  // Use line tracking
    // METHODS
    فراغ putsNoTracking(ثابت string& str) override { m_os.putsNoTracking(str); }
    فراغ puts(ثابت string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    فراغ putbs(ثابت string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    فراغ putfs(Ast_گره*, ثابت string& str) override { putbs(str); }
    فراغ putqs(Ast_گره*, ثابت string& str) override { putbs(str); }

عام:
    EmitVStreamVisitor(ثابت Ast_گره* nodep, std::ostream& os, منطقي tracking, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_بیرون_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_گره*>(nodep));
    }
    ~EmitVStreamVisitor() override = افتراضي;
};

//######################################################################
// EmitV class functions

فراغ V3EmitV::verilog_For_درخت(ثابت Ast_گره* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ خطأ, خطأ}; }
}

فراغ V3EmitV::debug_Verilog_For_درخت(ثابت Ast_گره* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ درست, درست}; }
}

std::string V3EmitV::debug_Verilog_For_درخت(ثابت Ast_گره* nodep) {
    std::stringstream ss;
    debug_Verilog_For_درخت(nodep, ss);
    أرجع ss.str();
}

فراغ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (Ast_گره_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), گره_File)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        إذا (vfilep && vfilep->tblockp()) {
            V3_بیرون_VFile از{vfilep->name()};
            از.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), از, خطأ};
        }
    }
}

فراغ V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_بیرون_VFile از{filename};
    EmitVFileVisitor{v3Global.rootp(), از, درست};
}
