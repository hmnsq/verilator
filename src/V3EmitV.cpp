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
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_ವೃಕ್ಷ* m_sentreep = ಶೂನ್ಯಸೂಚಕ;  // Domain for printing one a ALWAYS under a ACTIVE
    ತಾರ್ಕಿಕ m_suppressSemi = ಸುಳ್ಳು;  // Non-statement, don't print ;
    ತಾರ್ಕಿಕ m_suppressVarSemi = ಸುಳ್ಳು;  // Suppress emitting semicolon for AstVars
    ತಾರ್ಕಿಕ m_suppressSampled = ಸುಳ್ಳು;  // Suppress emitting sampled in assertion properties
    ತಾರ್ಕಿಕ m_arrayPost = ಸುಳ್ಳು;  // Print array information that goes after identifier (vs after)
    ತಾರ್ಕಿಕ m_prefixed = ನಿಜ;  // Whether constants need to be prefixed
    std::deque<Ast_ಗಂಟು_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_ಖಂಡ*, ಗಾತ್ರ_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& str) = 0;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putbs(ಸ್ಥಿರಾಂಕ string& str) = 0;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putfs(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& str) = 0;  // Fileline and node %% mark
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putqs(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& str) = 0;  // Fileline quiet w/o %% mark
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putsNoTracking(ಸ್ಥಿರಾಂಕ string& str) = 0;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putsQuoted(ಸ್ಥಿರಾಂಕ string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    ಶೂನ್ಯ iterateAndCommaConstNull(Ast_ಗಂಟು* nodep) {
        ಗಾಗಿ (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            ಆದರೆ (nodep->nextp()) puts(", ");
        }
    }
    ಶೂನ್ಯ emitPacked() {
        ಗಾಗಿ (Ast_ಗಂಟು_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    ಶೂನ್ಯ emit_Nodes_With_ಪಠ್ಯ(Ast_ಗಂಟು* nodesp, ತಾರ್ಕಿಕ tracking, ಸ್ಥಿರಾಂಕ std::string& separator) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಪಠ್ಯ* ಸ್ಥಿರಾಂಕ textp = VN_CAST(nodep, ಪಠ್ಯ_2)) {
                ಆದರೆ (tracking) {
                    puts(textp->ಪಠ್ಯ());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    putsNoTracking(textp->ಪಠ್ಯ());
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConst(nodep);
            }
            ಆದರೆ (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override { iterate_And_ಮುಂದಿನ_Const_Null(nodep->modulesp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪೋರ್ಟ್* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        ಸ್ಥಿರಾಂಕ std::string name = nodep->name().ಖಾಲಿಗಳು() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_Case_ಅಂಶ* nodep) override {
        ಆದರೆ (nodep->condsp()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->condsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            putbs("default");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->itemsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
            puts(";");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        ಆದರೆ (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        ಆದರೆ (nodep->name() == "") {
            putbs("begin\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        ಆದರೆ (nodep->name() == "") {
            putbs("fork\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap = VN_CAST(nodep->stmtsp(), ನಿಯೋಜಿಸು_W)) {
            ಆದರೆ (!ap->nextp()) {
                putfs(nodep, "assign ");
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ tcp = ap->timingControlp()) {
                    iterate_And_ಮುಂದಿನ_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_ಮುಂದಿನ_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_ಮುಂದಿನ_Const_Null(ap->rhsp());
                ಆದರೆ (!m_suppressSemi) puts(";\n");
                ಹಿಂತಿರುಗಿ;
            }
        }
        putfs(nodep, "always ");
        ಆದರೆ (m_sentreep) {
            iterate_And_ಮುಂದಿನ_Const_Null(m_sentreep);
        }  // In active
        ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (VN_IS(nodep, ನಿಯೋಜಿಸು_Force)) puts("force ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
        ಆದರೆ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        ಆದರೆ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
        ಆದರೆ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        puts("release ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        ಆದರೆ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBreak*) override {
        putbs("break");
        ಆದರೆ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        ಗಾಗಿ (Ast_ಗಂಟು* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            ಆದರೆ (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        putfs(nodep, "");
        ಆದರೆ (nodep->ಅಂಚು_Type() != VEdgeType::ET_ಬದಲಾಯಿತು) puts(nodep->ಅಂಚು_Type().verilogKwd());
        ಆದರೆ (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        putfs(nodep, "");
        ಆದರೆ (nodep->ಆದ್ಯತೆ_Pragma()) puts("priority ");
        ಆದರೆ (nodep->uniquePragma()) puts("unique ");
        ಆದರೆ (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts(")\n");
        ಆದರೆ (nodep->ತುಂಬಿದ_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            ಆದರೆ (nodep->ತುಂಬಿದ_Pragma()) puts(" full_case");
            ಆದರೆ (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        ಆದರೆ (nodep->condsp()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->condsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstContinue*) override {
        putbs("continue");
        ಆದರೆ (!m_suppressSemi) puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Decl*) override {}  // N/A
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverInc*) override {}  // N/A
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle*) override {}  // N/A

    ಶೂನ್ಯ ಭೇಟಿ(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        ಆದರೆ (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        ಆದರೆ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->optionsp());
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->binsp());
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
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cg_Option_ನಿಯೋಜಿಸು* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        ಆದರೆ (nodep->isArray()) {
            puts("[");
            ಆದರೆ (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        ಆದರೆ (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->transp()) {
            puts(" = ");
            ಗಾಗಿ (Ast_ಗಂಟು* setp = nodep->transp(); setp; setp = setp->nextp()) {
                ಆದರೆ (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            ಗಾಗಿ (Ast_ಗಂಟು* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                ಆದರೆ (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        ಆದರೆ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverWith* nodep) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        ಆದರೆ (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        ಆದರೆ (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        ಆದರೆ (!nodep->name().ಖಾಲಿಗಳು()) puts("." + nodep->name());
        puts(")");
        ಆದರೆ (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        ಆದರೆ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        ಆದರೆ (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        ಆದರೆ (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        ಆದರೆ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        ಆದರೆ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->optionsp());
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->binsp());
            puts("}\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(";\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cover_Trans_ಗಣ* nodep) override {
        puts("(");
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cover_Trans_ಅಂಶ* nodep) override { iterateChildrenConst(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->ಅಂಕೆ().ಗಮ್ಯ_String());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("string'(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Test_Plus_ವಾದಗಳು* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ_ಗಂಟು_Display(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* fileOrStrgp, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ,
                          Ast_ಗಂಟು* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಆದರೆ (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(ಪಠ್ಯ);
        ಗಾಗಿ (Ast_ಗಂಟು* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        ಭೇಟಿ_ಗಂಟು_Display(nodep, nodep->filep(), nodep->fmtp()->ಪಠ್ಯ(), nodep->fmtp()->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstElabDisplay* nodep) override {
        ಭೇಟಿ_ಗಂಟು_Display(nodep, ಶೂನ್ಯಸೂಚಕ, nodep->fmtp()->ಪಠ್ಯ(), nodep->fmtp()->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFScanF* nodep) override {
        ಭೇಟಿ_ಗಂಟು_Display(nodep, nodep->filep(), nodep->ಪಠ್ಯ(), nodep->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSScanF* nodep) override {
        ಭೇಟಿ_ಗಂಟು_Display(nodep, nodep->fromp(), nodep->ಪಠ್ಯ(), nodep->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormat* nodep) override {
        ಭೇಟಿ_ಗಂಟು_Display(nodep, nodep->lhsp(), nodep->fmtp()->ಪಠ್ಯ(), nodep->fmtp()->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಮ್ಯ_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        ಭೇಟಿ_ಗಂಟು_Display(nodep, ಶೂನ್ಯಸೂಚಕ, nodep->ಪಠ್ಯ(), nodep->exprsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->modep());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಆದರೆ (nodep->filep()) iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಆದರೆ (nodep->filep()) iterate_And_ಮುಂದಿನ_Const_Null(nodep->filep());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        // Allocate label number
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t n = m_labelNumbers.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = m_labelNumbers.emplace(nodep, n).ಎರಡನೇ;
        UASSERT_OBJ(new_ನಮೂದು, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::ಗಮ್ಯ_string(n) + "\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_labelNumbers.ಕಂಡುಹಿಡಿ(nodep->blockp());
        ಸ್ಥಿರಾಂಕ std::string label
            = it != m_labelNumbers.ಕೊನೆ() ? "label" + std::ಗಮ್ಯ_string(it->ಎರಡನೇ) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ಆದರೆ (nodep->filenamep()) iterate_And_ಮುಂದಿನ_Const_Null(nodep->filenamep());
        putbs(", ");
        ಆದರೆ (nodep->memp()) iterate_And_ಮುಂದಿನ_Const_Null(nodep->memp());
        ಆದರೆ (nodep->lsbp()) {
            putbs(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lsbp());
        }
        ಆದರೆ (nodep->msbp()) {
            putbs(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_ಮುಂದಿನ_Const_Null(testp->nextp());
            puts("end\n");
            ಹಿಂತಿರುಗಿ;
        }
        // Special case when the AstLoopTest is last for output readability
        ಆದರೆ (Ast_ಗಂಟು* lastp = nodep->stmtsp()) {
            ಆಗ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = lastp->nextp()) lastp = nextp;
            ಆದರೆ (AstLoopTest* ಸ್ಥಿರಾಂಕ testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                ಗಾಗಿ (Ast_ಗಂಟು* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                ಹಿಂತಿರುಗಿ;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        putfs(nodep, "");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, If)) {
            ಆದರೆ (ifp->ಆದ್ಯತೆ_Pragma()) puts("priority ");
            ಆದರೆ (ifp->uniquePragma()) puts("unique ");
            ಆದರೆ (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        ಆದರೆ (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        ಆದರೆ (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->ticksp());
            ಆದರೆ (nodep->sentreep()) {
                puts(", ");
                iterate_And_ಮುಂದಿನ_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSampled* nodep) override {
        ಆದರೆ (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        ಆದರೆ (nodep->sentreep()) {
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        ಆದರೆ (nodep->sentreep()) {
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        ಆದರೆ (nodep->sentreep()) {
            puts(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ* nodep) override { putfs(nodep, "$finish;\n"); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ_ಖಂಡ* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().ಖಾಲಿಗಳು();
        puts(nodep->ಪೂರ್ವಪ್ರತ್ಯಯ());
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), ನಿಜ, nodep->separator());
        puts(nodep->suffix());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), ನಿಜ, "");
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), ನಿಜ, "");
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), ಸುಳ್ಳು, "");
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_ಪಠ್ಯ(nodep->nodesp(), ಸುಳ್ಳು, "");
        puts(")");
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->resultp());
        puts(");\n");
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ emitVerilogFormat(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& format, Ast_ಗಂಟು* lhsp = ಶೂನ್ಯಸೂಚಕ,
                                   Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsp = ಶೂನ್ಯಸೂಚಕ, Ast_ಗಂಟು* thsp = ಶೂನ್ಯಸೂಚಕ,
                                   Ast_ಗಂಟು* fhsp = ಶೂನ್ಯಸೂಚಕ) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        ತಾರ್ಕಿಕ ಒಳ_Pct = ಸುಳ್ಳು;
        putbs("");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : format) {
            ಆದರೆ (!ಒಳ_Pct && c == '%') {
                ಒಳ_Pct = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಒಳ_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } ಇಲ್ಲದಿದ್ದರೆ {  // Format character
                ಒಳ_Pct = ಸುಳ್ಳು;
                ಆಯ್ಕೆ (c) {
                ಸ್ಥಿತಿ '%': puts("%"); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'f': putfs(nodep, ""); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'k': putbs(""); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ಮುಂದಿನ_Const_Null(lhsp);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ಮುಂದಿನ_Const_Null(rhsp);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ಮುಂದಿನ_Const_Null(thsp);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ಮುಂದಿನ_Const_Null(fhsp);
                    ನಿಲ್ಲಿಸು;
                }
                ಸ್ಥಿತಿ 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_ಮುಂದಿನ_Const_Null(nodep->dtypep());
                    ನಿಲ್ಲಿಸು;
                }
                ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); ನಿಲ್ಲಿಸು;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        ಆದರೆ (nodep->dimp()) {
            putbs(", ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Array_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ಅಂಶ)) {
            ಆದರೆ (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_ಗಮ್ಯ_Str(itemp->ಸೂಚ್ಯಂಕ()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        putbs("(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsep());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());
        puts("]");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRange* nodep) override {
        puts("[");
        ಆದರೆ (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಎಡ_Const()));
            puts(":");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಬಲ_Const()));
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        ಪೂರ್ಣಾಂಕ ಸ್ಥಾನಾಂತರ = 0;
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->fromp()->dtypep();
        ಆದರೆ (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ಸ್ಥಿರಾಂಕ basicDtypep = VN_AS(dtypep, BasicDType);
            ಸ್ಥಾನಾಂತರ = basicDtypep->lo();
        }
        puts("[");
        ಆದರೆ (VN_IS(nodep->lsbp(), Const)) {
            ಆದರೆ (nodep->ಅಗಲ_Const() == 1) {
                puts(cvt_ಗಮ್ಯ_Str(VN_AS(nodep->lsbp(), Const)->ಗಮ್ಯ_SInt() + ಸ್ಥಾನಾಂತರ));
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts(cvt_ಗಮ್ಯ_Str(VN_AS(nodep->lsbp(), Const)->ಗಮ್ಯ_SInt() + nodep->ಅಗಲ_Const() + ಸ್ಥಾನಾಂತರ
                              - 1));
                puts(":");
                puts(cvt_ಗಮ್ಯ_Str(VN_AS(nodep->lsbp(), Const)->ಗಮ್ಯ_SInt() + ಸ್ಥಾನಾಂತರ));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->lsbp());
            ಆದರೆ (ಸ್ಥಾನಾಂತರ != 0) {
                puts(" + ");
                puts(cvt_ಗಮ್ಯ_Str(ಸ್ಥಾನಾಂತರ));
            }
            putfs(nodep, "+:");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಅಗಲ_Const()));
        }
        puts("]");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->fromp());
        puts(cvt_ಗಮ್ಯ_Str(nodep->declRange()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstThisRef* /*nodep*/) override { puts("this"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        ಆದರೆ (nodep->userType() == VAssertType::OBSERVED_DEFERRED_ತಕ್ಷಣ) {
            puts("#0 ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->userType() == VAssertType::FINAL_DEFERRED_ತಕ್ಷಣ) {
            puts("final ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        ಆದರೆ (AstSampled* ಸ್ಥಿರಾಂಕ sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_ಮುಂದಿನ_Const_Null(sampledp->exprp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->propp());
        }
        ಆದರೆ (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->passsp());
            puts("end\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(");\n");
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssert* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_ಮುಂದಿನ_Const_Null(assertp->failsp());
            puts("end\n");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAssertIntrinsic* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_ಮುಂದಿನ_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocArrayDType* nodep) override {
        ಆದರೆ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ಸುಳ್ಳು;
            puts("[");
            iterateConst(nodep->ಕೀಲಿ_DTypep());
            puts("]");
            m_arrayPost = ನಿಜ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBasicDType* nodep) override {
        ಆದರೆ (m_arrayPost) ಹಿಂತಿರುಗಿ;
        putfs(nodep, nodep->prettyName());
        ಆದರೆ (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        ಆದರೆ (nodep->keyword().isIntNumeric() && !nodep->keyword().is_ಬಿಟ್_Logic()) ಹಿಂತಿರುಗಿ;
        emitPacked();
        ಆದರೆ (nodep->rangep()) {
            puts(" ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->rangep());
            puts(" ");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isRanged()) {
            puts(" [");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಎಡ()));
            puts(":");
            puts(cvt_ಗಮ್ಯ_Str(nodep->ಬಲ()));
            puts("] ");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstDType* nodep) override {
        ಆದರೆ (m_arrayPost) ಹಿಂತಿರುಗಿ;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDynArrayDType* nodep) override {
        ಆದರೆ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEnumDType* nodep) override {
        ಆದರೆ (m_arrayPost) ಹಿಂತಿರುಗಿ;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->itemsp());
        puts("}");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        ಆದರೆ (nodep->nextp()) puts(",");
        puts("\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Array_DType* nodep) override {
        ಆದರೆ (!m_arrayPost) {
            ಆದರೆ (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            }
            iterateConst(nodep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = ಸುಳ್ಳು;
                iterate_And_ಮುಂದಿನ_Const_Null(nodep->rangep());
                m_arrayPost = ನಿಜ;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        ಆದರೆ (m_arrayPost) {
            puts(" (");
            ಆದರೆ (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                puts("????");
            }
            puts(")");
            ಹಿಂತಿರುಗಿ;
        }
        puts(nodep->ifaceName());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        ಆದರೆ (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(nodep->classp()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        ಆದರೆ (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("????");
        }
        puts(");\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        ಆದರೆ (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = ನಿಜ;
            iterateConst(nodep->varp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(nodep->prettyName());
        }
        ಆದರೆ (nodep->nextp()) puts(", ");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಆದರೆ (m_arrayPost) ಹಿಂತಿರುಗಿ;
        puts(nodep->verilogKwd() + " ");
        ಆದರೆ (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            ಗಾಗಿ (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberDType* nodep) override {
        ಆದರೆ (m_arrayPost) ಹಿಂತಿರುಗಿ;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರತಿ_DType* nodep) override {
        ಆದರೆ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ಸುಳ್ಳು;
            puts("[$");
            ಆದರೆ (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = ನಿಜ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } ಇಲ್ಲದಿದ್ದರೆ {
            putfs(nodep, nodep->prettyName());
        }
        ಆದರೆ (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->argsp());
        puts(")");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArg* nodep) override { iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Print_ಸಮಯ_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPropSpec* nodep) override {
        ಆದರೆ (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            ಗಾಗಿ (Ast_ಗಂಟು* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                ಆದರೆ (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        ಆದರೆ (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        ಆದರೆ (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        ಆದರೆ (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstAbortOn* nodep) override {
        puts(nodep->ಬಗೆ().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        ಆದರೆ (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ನಿಜ;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (nodep->varp()) {
                ಆದರೆ (nodep->self_ಸೂಚಕ().is_ಖಾಲಿಗಳು()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    putfs(nodep, nodep->self_ಸೂಚಕ().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                putfs(nodep, nodep->name());
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        ಆದರೆ (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(nodep->prettyName());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override { putfs(nodep, nodep->ಅಂಕೆ().ascii(m_prefixed, ನಿಜ)); }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುದಿ_Scope* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override { iterateChildrenConst(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = ಸುಳ್ಳು;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = ನಿಜ;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        ಆದರೆ (nodep->is_ಚಕ್ರ_Delay()) {
            puts("##");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = ಸುಳ್ಳು;
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateConst(nodep->lhsp());
        }
        ಆದರೆ (!m_suppressSemi) {
            puts(";\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(" ");
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        ಆದರೆ (nodep->stmtsp()) {
            puts(" ");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(";\n");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceInc*) override {}
    // NOPs
    ಶೂನ್ಯ ಭೇಟಿ(AstPragma*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstStmtPragma*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCell*) override {}  // Handled outside the Visit class
    // Default
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        ಆದರೆ (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ EmitVBaseVisitorConst(ತಾರ್ಕಿಕ suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Emit to an output file

ವರ್ಗ EmitVFileVisitor final : ಸಾರ್ವಜನಿಕ EmitVBaseVisitorConst {
    // STATE
    V3_ಹೊರಹೋಗು_VFile& m_ದ;  // The output file
    // METHODS
    ಶೂನ್ಯ putsNoTracking(ಸ್ಥಿರಾಂಕ string& str) override { m_ದ.putsNoTracking(str); }
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& str) override { m_ದ.puts(str); }
    ಶೂನ್ಯ putbs(ಸ್ಥಿರಾಂಕ string& str) override { m_ದ.putbs(str); }
    ಶೂನ್ಯ putfs(Ast_ಗಂಟು*, ಸ್ಥಿರಾಂಕ string& str) override { putbs(str); }
    ಶೂನ್ಯ putqs(Ast_ಗಂಟು*, ಸ್ಥಿರಾಂಕ string& str) override { putbs(str); }

ಸಾರ್ವಜನಿಕ:
    EmitVFileVisitor(Ast_ಗಂಟು* nodep, V3_ಹೊರಹೋಗು_VFile& ದ, ತಾರ್ಕಿಕ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_ದ{ದ} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

ವರ್ಗ EmitVStreamVisitor final : ಸಾರ್ವಜನಿಕ EmitVBaseVisitorConst {
    // STATE
    V3_ಹೊರಹೋಗು_ಹರಿವು m_os;  // The output stream formatter
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_tracking;  // Use line tracking
    // METHODS
    ಶೂನ್ಯ putsNoTracking(ಸ್ಥಿರಾಂಕ string& str) override { m_os.putsNoTracking(str); }
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    ಶೂನ್ಯ putbs(ಸ್ಥಿರಾಂಕ string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    ಶೂನ್ಯ putfs(Ast_ಗಂಟು*, ಸ್ಥಿರಾಂಕ string& str) override { putbs(str); }
    ಶೂನ್ಯ putqs(Ast_ಗಂಟು*, ಸ್ಥಿರಾಂಕ string& str) override { putbs(str); }

ಸಾರ್ವಜನಿಕ:
    EmitVStreamVisitor(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, std::ostream& os, ತಾರ್ಕಿಕ tracking, ತಾರ್ಕಿಕ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_ಹೊರಹೋಗು_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ಗಂಟು*>(nodep));
    }
    ~EmitVStreamVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// EmitV class functions

ಶೂನ್ಯ V3EmitV::verilog_For_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ಸುಳ್ಳು, ಸುಳ್ಳು}; }
}

ಶೂನ್ಯ V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ನಿಜ, ನಿಜ}; }
}

std::string V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    std::stringstream ss;
    debug_Verilog_For_ವೃಕ್ಷ(nodep, ss);
    ಹಿಂತಿರುಗಿ ss.str();
}

ಶೂನ್ಯ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ಗಾಗಿ (Ast_ಗಂಟು_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ಗಂಟು_File)) {
        AstVFile* ಸ್ಥಿರಾಂಕ vfilep = VN_CAST(filep, VFile);
        ಆದರೆ (vfilep && vfilep->tblockp()) {
            V3_ಹೊರಹೋಗು_VFile ದ{vfilep->name()};
            ದ.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), ದ, ಸುಳ್ಳು};
        }
    }
}

ಶೂನ್ಯ V3EmitV::debugEmitV(ಸ್ಥಿರಾಂಕ string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_ಹೊರಹೋಗು_VFile ದ{filename};
    EmitVFileVisitor{v3Global.rootp(), ದ, ನಿಜ};
}
