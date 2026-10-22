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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

ਵਰਗ EmitVBaseVisitorConst VL_NOT_FINAL : ਜਨਤਕ VNVisitorConst {
    // STATE - across all visitors
    ਅਚਲ ਤਾਰਕਿਕ m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_ਰੁੱਖ* m_sentreep = ਖਾਲੀ_ਸੂਚਕ;  // Domain for printing one a ALWAYS under a ACTIVE
    ਤਾਰਕਿਕ m_suppressSemi = ਝੂਠ;  // Non-statement, don't print ;
    ਤਾਰਕਿਕ m_suppressVarSemi = ਝੂਠ;  // Suppress emitting semicolon for AstVars
    ਤਾਰਕਿਕ m_suppressSampled = ਝੂਠ;  // Suppress emitting sampled in assertion properties
    ਤਾਰਕਿਕ m_arrayPost = ਝੂਠ;  // Print array information that goes after identifier (vs after)
    ਤਾਰਕਿਕ m_prefixed = ਸੱਚ;  // Whether constants need to be prefixed
    std::deque<Ast_ਗੰਢ_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<Ast_Jump_ਖੰਡ*, ਮਾਪ_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    ਆਭਾਸੀ ਖਾਲੀ puts(ਅਚਲ string& str) = 0;
    ਆਭਾਸੀ ਖਾਲੀ putbs(ਅਚਲ string& str) = 0;
    ਆਭਾਸੀ ਖਾਲੀ putfs(Ast_ਗੰਢ* nodep, ਅਚਲ string& str) = 0;  // Fileline and node %% mark
    ਆਭਾਸੀ ਖਾਲੀ putqs(Ast_ਗੰਢ* nodep, ਅਚਲ string& str) = 0;  // Fileline quiet w/o %% mark
    ਆਭਾਸੀ ਖਾਲੀ putsNoTracking(ਅਚਲ string& str) = 0;
    ਆਭਾਸੀ ਖਾਲੀ putsQuoted(ਅਚਲ string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    ਖਾਲੀ iterateAndCommaConstNull(Ast_ਗੰਢ* nodep) {
        ਲਈ (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            ਜੇ (nodep->nextp()) puts(", ");
        }
    }
    ਖਾਲੀ emitPacked() {
        ਲਈ (Ast_ਗੰਢ_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    ਖਾਲੀ emit_Nodes_With_ਪਾਠ(Ast_ਗੰਢ* nodesp, ਤਾਰਕਿਕ tracking, ਅਚਲ std::string& separator) {
        ਲਈ (Ast_ਗੰਢ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            ਜੇ (ਅਚਲ Ast_ਪਾਠ* ਅਚਲ textp = VN_CAST(nodep, ਪਾਠ_2)) {
                ਜੇ (tracking) {
                    puts(textp->ਪਾਠ());
                } ਨਹੀਂਤਾਂ {
                    putsNoTracking(textp->ਪਾਠ());
                }
            } ਨਹੀਂਤਾਂ {
                iterateConst(nodep);
            }
            ਜੇ (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override { iterate_And_ਅਗਲਾ_Const_Null(nodep->modulesp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪੋਰਟ* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        ਅਚਲ std::string name = nodep->name().ਖਾਲੀ_ਥਾਵਾਂ() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts(")\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_Case_ਵਸਤੂ* nodep) override {
        ਜੇ (nodep->condsp()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->condsp());
        } ਨਹੀਂਤਾਂ {
            putbs("default");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->itemsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Const_Null(nodep->initsp());
            puts(";");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
            puts(";");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->incsp());
        }
        puts(") begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        ਜੇ (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        ਜੇ (nodep->name() == "") {
            putbs("begin\n");
        } ਨਹੀਂਤਾਂ {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        ਜੇ (nodep->name() == "") {
            putbs("fork\n");
        } ਨਹੀਂਤਾਂ {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = VN_CAST(nodep->stmtsp(), ਸੌਂਪੋ_W)) {
            ਜੇ (!ap->nextp()) {
                putfs(nodep, "assign ");
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ tcp = ap->timingControlp()) {
                    iterate_And_ਅਗਲਾ_Const_Null(tcp);
                    putbs(" ");
                }
                iterate_And_ਅਗਲਾ_Const_Null(ap->lhsp());
                putbs(" = ");
                iterate_And_ਅਗਲਾ_Const_Null(ap->rhsp());
                ਜੇ (!m_suppressSemi) puts(";\n");
                ਵਾਪਸ;
            }
        }
        putfs(nodep, "always ");
        ਜੇ (m_sentreep) {
            iterate_And_ਅਗਲਾ_Const_Null(m_sentreep);
        }  // In active
        ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->sentreep());
        }
        putbs(" begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (VN_IS(nodep, ਸੌਂਪੋ_Force)) puts("force ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
        ਜੇ (!m_suppressSemi) puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        ਜੇ (!m_suppressSemi) puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        putfs(nodep, "continuous assign ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        putbs(" = ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
        ਜੇ (!m_suppressSemi) puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        puts("release ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        ਜੇ (!m_suppressSemi) puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstBreak*) override {
        putbs("break");
        ਜੇ (!m_suppressSemi) puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        ਲਈ (Ast_ਗੰਢ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            ਜੇ (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        putfs(nodep, "");
        ਜੇ (nodep->ਕਿਨਾਰਾ_Type() != VEdgeType::ET_ਬਦਲਿਆ) puts(nodep->ਕਿਨਾਰਾ_Type().verilogKwd());
        ਜੇ (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        putfs(nodep, "");
        ਜੇ (nodep->ਤਰਜੀਹ_Pragma()) puts("priority ");
        ਜੇ (nodep->uniquePragma()) puts("unique ");
        ਜੇ (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts(")\n");
        ਜੇ (nodep->ਭਰਿਆ_Pragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            ਜੇ (nodep->ਭਰਿਆ_Pragma()) puts(" full_case");
            ਜੇ (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        ਜੇ (nodep->condsp()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->condsp());
        } ਨਹੀਂਤਾਂ {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstContinue*) override {
        putbs("continue");
        ਜੇ (!m_suppressSemi) puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Decl*) override {}  // N/A
    ਖਾਲੀ ਦੇਖੋ(AstCoverInc*) override {}  // N/A
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle*) override {}  // N/A

    ਖਾਲੀ ਦੇਖੋ(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        ਜੇ (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        ਜੇ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->optionsp());
            iterate_And_ਅਗਲਾ_Const_Null(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    ਖਾਲੀ emitCoverageOption(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ typeOption, VCoverOptionType optType,
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cg_Option_ਸੌਂਪੋ* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        ਜੇ (nodep->isArray()) {
            puts("[");
            ਜੇ (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        ਜੇ (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->transp()) {
            puts(" = ");
            ਲਈ (Ast_ਗੰਢ* setp = nodep->transp(); setp; setp = setp->nextp()) {
                ਜੇ (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            ਲਈ (Ast_ਗੰਢ* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                ਜੇ (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        ਜੇ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverWith* nodep) override {
        ਅਚਲ ਤਾਰਕਿਕ rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        ਜੇ (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        ਜੇ (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        ਜੇ (!nodep->name().ਖਾਲੀ_ਥਾਵਾਂ()) puts("." + nodep->name());
        puts(")");
        ਜੇ (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        ਜੇ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        ਜੇ (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        ਜੇ (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        ਜੇ (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        ਜੇ (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->optionsp());
            iterate_And_ਅਗਲਾ_Const_Null(nodep->binsp());
            puts("}\n");
        } ਨਹੀਂਤਾਂ {
            puts(";\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cover_Trans_ਸੈੱਟ* nodep) override {
        puts("(");
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cover_Trans_ਵਸਤੂ* nodep) override { iterateChildrenConst(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        ਜੇ (AstConst* ਅਚਲ lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->ਅੰਕ().ਮੰਜ਼ਿਲ_String());
        } ਨਹੀਂਤਾਂ {
            puts("string'(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
            puts(")");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Test_Plus_ਦਲੀਲਾਂ* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ_ਗੰਢ_Display(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* fileOrStrgp, ਅਚਲ string& ਪਾਠ,
                          Ast_ਗੰਢ* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ਜੇ (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(ਪਾਠ);
        ਲਈ (Ast_ਗੰਢ* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        ਦੇਖੋ_ਗੰਢ_Display(nodep, nodep->filep(), nodep->fmtp()->ਪਾਠ(), nodep->fmtp()->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstElabDisplay* nodep) override {
        ਦੇਖੋ_ਗੰਢ_Display(nodep, ਖਾਲੀ_ਸੂਚਕ, nodep->fmtp()->ਪਾਠ(), nodep->fmtp()->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFScanF* nodep) override {
        ਦੇਖੋ_ਗੰਢ_Display(nodep, nodep->filep(), nodep->ਪਾਠ(), nodep->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        ਦੇਖੋ_ਗੰਢ_Display(nodep, nodep->fromp(), nodep->ਪਾਠ(), nodep->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormat* nodep) override {
        ਦੇਖੋ_ਗੰਢ_Display(nodep, nodep->lhsp(), nodep->fmtp()->ਪਾਠ(), nodep->fmtp()->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੰਜ਼ਿਲ_String_N* nodep) override { iterateConst(nodep->lhsp()); }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        ਦੇਖੋ_ਗੰਢ_Display(nodep, ਖਾਲੀ_ਸੂਚਕ, nodep->ਪਾਠ(), nodep->exprsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filenamep());
        putbs(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->modep());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->filenamep());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ਜੇ (nodep->filep()) iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ਜੇ (nodep->filep()) iterate_And_ਅਗਲਾ_Const_Null(nodep->filep());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        // Allocate label number
        ਅਚਲ ਮਾਪ_t n = m_labelNumbers.ਮਾਪ();
        ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = m_labelNumbers.emplace(nodep, n).ਦੂਜਾ;
        UASSERT_OBJ(new_ਇੰਦਰਾਜ, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::ਮੰਜ਼ਿਲ_string(n) + "\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        puts("end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_labelNumbers.ਲੱਭੋ(nodep->blockp());
        ਅਚਲ std::string label
            = it != m_labelNumbers.ਸਿਰਾ() ? "label" + std::ਮੰਜ਼ਿਲ_string(it->ਦੂਜਾ) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        ਜੇ (nodep->filenamep()) iterate_And_ਅਗਲਾ_Const_Null(nodep->filenamep());
        putbs(", ");
        ਜੇ (nodep->memp()) iterate_And_ਅਗਲਾ_Const_Null(nodep->memp());
        ਜੇ (nodep->lsbp()) {
            putbs(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lsbp());
        }
        ਜੇ (nodep->msbp()) {
            putbs(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->msbp());
        }
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprsp());
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->countp());
        puts(") begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterate_And_ਅਗਲਾ_Const_Null(testp->nextp());
            puts("end\n");
            ਵਾਪਸ;
        }
        // Special case when the AstLoopTest is last for output readability
        ਜੇ (Ast_ਗੰਢ* lastp = nodep->stmtsp()) {
            ਜਦੋਂ (Ast_ਗੰਢ* ਅਚਲ nextp = lastp->nextp()) lastp = nextp;
            ਜੇ (AstLoopTest* ਅਚਲ testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                ਲਈ (Ast_ਗੰਢ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                ਵਾਪਸ;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        iterate_And_ਅਗਲਾ_Const_Null(nodep->contsp());
        putfs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        puts(")) break;\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        putfs(nodep, "");
        ਜੇ (ਅਚਲ AstIf* ਅਚਲ ifp = VN_CAST(nodep, If)) {
            ਜੇ (ifp->ਤਰਜੀਹ_Pragma()) puts("priority ");
            ਜੇ (ifp->uniquePragma()) puts("unique ");
            ਜੇ (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        puts(") begin\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        ਜੇ (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        ਜੇ (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->ticksp());
            ਜੇ (nodep->sentreep()) {
                puts(", ");
                iterate_And_ਅਗਲਾ_Const_Null(nodep->sentreep());
            }
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSampled* nodep) override {
        ਜੇ (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
            puts(")");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        ਜੇ (nodep->sentreep()) {
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        ਜੇ (nodep->sentreep()) {
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        ਜੇ (nodep->sentreep()) {
            puts(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->sentreep());
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ* nodep) override { putfs(nodep, "$finish;\n"); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ_Fork* nodep) override { putfs(nodep, "$finish;\n"); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ_ਖੰਡ* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().ਖਾਲੀ_ਥਾਵਾਂ();
        puts(nodep->ਅਗੇਤਰ());
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), ਸੱਚ, nodep->separator());
        puts(nodep->suffix());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), ਸੱਚ, "");
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), ਸੱਚ, "");
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), ਝੂਠ, "");
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emit_Nodes_With_ਪਾਠ(nodep->nodesp(), ਝੂਠ, "");
        puts(")");
    }

    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        putbs(", ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->resultp());
        puts(");\n");
    }

    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    ਆਭਾਸੀ ਖਾਲੀ emitVerilogFormat(Ast_ਗੰਢ* nodep, ਅਚਲ string& format, Ast_ਗੰਢ* lhsp = ਖਾਲੀ_ਸੂਚਕ,
                                   Ast_ਗੰਢ* ਅਚਲ rhsp = ਖਾਲੀ_ਸੂਚਕ, Ast_ਗੰਢ* thsp = ਖਾਲੀ_ਸੂਚਕ,
                                   Ast_ਗੰਢ* fhsp = ਖਾਲੀ_ਸੂਚਕ) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        ਤਾਰਕਿਕ ਵਿੱਚ_Pct = ਝੂਠ;
        putbs("");
        ਲਈ (ਅਚਲ ਅੱਖਰ c : format) {
            ਜੇ (!ਵਿੱਚ_Pct && c == '%') {
                ਵਿੱਚ_Pct = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (!ਵਿੱਚ_Pct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } ਨਹੀਂਤਾਂ {  // Format character
                ਵਿੱਚ_Pct = ਝੂਠ;
                ਚੋਣ (c) {
                ਹਾਲਤ '%': puts("%"); ਰੋਕੋ;
                ਹਾਲਤ 'f': putfs(nodep, ""); ਰੋਕੋ;
                ਹਾਲਤ 'k': putbs(""); ਰੋਕੋ;
                ਹਾਲਤ 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ਅਗਲਾ_Const_Null(lhsp);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ਅਗਲਾ_Const_Null(rhsp);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ਅਗਲਾ_Const_Null(thsp);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterate_And_ਅਗਲਾ_Const_Null(fhsp);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterate_And_ਅਗਲਾ_Const_Null(nodep->dtypep());
                    ਰੋਕੋ;
                }
                ਮੂਲ: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); ਰੋਕੋ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        ਜੇ (nodep->dimp()) {
            putbs(", ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->dimp());
        }
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        ਲਈ (ਅਚਲ Ast_Cons_Array_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ਵਸਤੂ)) {
            ਜੇ (itemp != nodep->itemsp()) putbs(", ");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(itemp->ਸੂਚਕਾਂਕ()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        putbs("(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        putfs(nodep, " ? ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thenp());
        putbs(" : ");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsep());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstInsideRange* nodep) override {
        puts("[");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lhsp());
        puts(":");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());
        puts("]");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRange* nodep) override {
        puts("[");
        ਜੇ (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਖੱਬਾ_Const()));
            puts(":");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਸੱਜਾ_Const()));
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->leftp());
            puts(":");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->rightp());
        }
        puts("]");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        ਪੂਰਨ ਵਿੱਥ = 0;
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->fromp()->dtypep();
        ਜੇ (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ਅਚਲ basicDtypep = VN_AS(dtypep, BasicDType);
            ਵਿੱਥ = basicDtypep->lo();
        }
        puts("[");
        ਜੇ (VN_IS(nodep->lsbp(), Const)) {
            ਜੇ (nodep->ਚੌੜਾਈ_Const() == 1) {
                puts(cvt_ਮੰਜ਼ਿਲ_Str(VN_AS(nodep->lsbp(), Const)->ਮੰਜ਼ਿਲ_SInt() + ਵਿੱਥ));
            } ਨਹੀਂਤਾਂ {
                puts(cvt_ਮੰਜ਼ਿਲ_Str(VN_AS(nodep->lsbp(), Const)->ਮੰਜ਼ਿਲ_SInt() + nodep->ਚੌੜਾਈ_Const() + ਵਿੱਥ
                              - 1));
                puts(":");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(VN_AS(nodep->lsbp(), Const)->ਮੰਜ਼ਿਲ_SInt() + ਵਿੱਥ));
            }
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->lsbp());
            ਜੇ (ਵਿੱਥ != 0) {
                puts(" + ");
                puts(cvt_ਮੰਜ਼ਿਲ_Str(ਵਿੱਥ));
            }
            putfs(nodep, "+:");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਚੌੜਾਈ_Const()));
        }
        puts("]");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        iterate_And_ਅਗਲਾ_Const_Null(nodep->fromp());
        puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->declRange()));
    }
    ਖਾਲੀ ਦੇਖੋ(AstThisRef* /*nodep*/) override { puts("this"); }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        ਜੇ (nodep->userType() == VAssertType::OBSERVED_DEFERRED_ਤੁਰੰਤ) {
            puts("#0 ");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->userType() == VAssertType::FINAL_DEFERRED_ਤੁਰੰਤ) {
            puts("final ");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        ਜੇ (AstSampled* ਅਚਲ sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterate_And_ਅਗਲਾ_Const_Null(sampledp->exprp());
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->propp());
        }
        ਜੇ (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->passsp());
            puts("end\n");
        } ਨਹੀਂਤਾਂ {
            puts(");\n");
        }

        ਜੇ (ਅਚਲ AstAssert* ਅਚਲ assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterate_And_ਅਗਲਾ_Const_Null(assertp->failsp());
            puts("end\n");
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstAssertIntrinsic* ਅਚਲ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterate_And_ਅਗਲਾ_Const_Null(assertp->failsp());
            puts("end\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocArrayDType* nodep) override {
        ਜੇ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ਝੂਠ;
            puts("[");
            iterateConst(nodep->ਕੁੰਜੀ_DTypep());
            puts("]");
            m_arrayPost = ਸੱਚ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstBasicDType* nodep) override {
        ਜੇ (m_arrayPost) ਵਾਪਸ;
        putfs(nodep, nodep->prettyName());
        ਜੇ (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        ਜੇ (nodep->keyword().isIntNumeric() && !nodep->keyword().is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic()) ਵਾਪਸ;
        emitPacked();
        ਜੇ (nodep->rangep()) {
            puts(" ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->rangep());
            puts(" ");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isRanged()) {
            puts(" [");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਖੱਬੇ()));
            puts(":");
            puts(cvt_ਮੰਜ਼ਿਲ_Str(nodep->ਸੱਜੇ()));
            puts("] ");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstDType* nodep) override {
        ਜੇ (m_arrayPost) ਵਾਪਸ;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstDynArrayDType* nodep) override {
        ਜੇ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ਨਹੀਂਤਾਂ {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstEnumDType* nodep) override {
        ਜੇ (m_arrayPost) ਵਾਪਸ;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->itemsp());
        puts("}");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        ਜੇ (nodep->nextp()) puts(",");
        puts("\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Array_DType* nodep) override {
        ਜੇ (!m_arrayPost) {
            ਜੇ (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ਧੱਕੋ_ਪਿੱਠ(nodep);
            }
            iterateConst(nodep->subDTypep());
        } ਨਹੀਂਤਾਂ {
            ਜੇ (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = ਝੂਠ;
                iterate_And_ਅਗਲਾ_Const_Null(nodep->rangep());
                m_arrayPost = ਸੱਚ;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        ਜੇ (m_arrayPost) {
            puts(" (");
            ਜੇ (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } ਨਹੀਂਤਾਂ {
                puts("????");
            }
            puts(")");
            ਵਾਪਸ;
        }
        puts(nodep->ifaceName());
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        ਜੇ (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } ਨਹੀਂਤਾਂ {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::ਅਗੇਤਰ_Name_Protect(nodep->classp()));
    }
    ਖਾਲੀ ਦੇਖੋ(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    ਖਾਲੀ ਦੇਖੋ(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        ਜੇ (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } ਨਹੀਂਤਾਂ {
            puts("????");
        }
        puts(");\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        ਜੇ (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = ਸੱਚ;
            iterateConst(nodep->varp());
        } ਨਹੀਂਤਾਂ {
            puts(nodep->prettyName());
        }
        ਜੇ (nodep->nextp()) puts(", ");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        ਜੇ (m_arrayPost) ਵਾਪਸ;
        puts(nodep->verilogKwd() + " ");
        ਜੇ (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            ਲਈ (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberDType* nodep) override {
        ਜੇ (m_arrayPost) ਵਾਪਸ;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਕਤਾਰ_DType* nodep) override {
        ਜੇ (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = ਝੂਠ;
            puts("[$");
            ਜੇ (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = ਸੱਚ;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } ਨਹੀਂਤਾਂ {
            putfs(nodep, nodep->prettyName());
        }
        ਜੇ (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterate_And_ਅਗਲਾ_Const_Null(nodep->argsp());
        puts(")");
    }
    ਖਾਲੀ ਦੇਖੋ(AstArg* nodep) override { iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp()); }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    ਖਾਲੀ ਦੇਖੋ(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Print_ਸਮਾਂ_Scale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstPropSpec* nodep) override {
        ਜੇ (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            ਲਈ (Ast_ਗੰਢ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                ਜੇ (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        ਜੇ (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        ਜੇ (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        ਜੇ (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    ਖਾਲੀ ਦੇਖੋ(AstAbortOn* nodep) override {
        puts(nodep->ਕਿਸਮ().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    ਖਾਲੀ ਦੇਖੋ(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        ਜੇ (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = ਸੱਚ;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->varp()) {
                ਜੇ (nodep->self_ਸੂਚਕ().is_ਖਾਲੀ_ਥਾਵਾਂ()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } ਨਹੀਂਤਾਂ {
                    putfs(nodep, nodep->self_ਸੂਚਕ().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } ਨਹੀਂਤਾਂ {
                putfs(nodep, nodep->name());
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        ਜੇ (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } ਨਹੀਂਤਾਂ {
            puts(nodep->prettyName());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override { putfs(nodep, nodep->ਅੰਕ().ascii(m_prefixed, ਸੱਚ)); }
    ਖਾਲੀ ਦੇਖੋ(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਿਖਰ_Scope* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override { iterateChildrenConst(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = ਝੂਠ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = ਸੱਚ;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        ਜੇ (nodep->is_ਚੱਕਰ_Delay()) {
            puts("##");
        } ਨਹੀਂਤਾਂ {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = ਝੂਠ;
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } ਨਹੀਂਤਾਂ {
            iterateConst(nodep->lhsp());
        }
        ਜੇ (!m_suppressSemi) {
            puts(";\n");
        } ਨਹੀਂਤਾਂ {
            puts(" ");
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        ਜੇ (nodep->stmtsp()) {
            puts(" ");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        } ਨਹੀਂਤਾਂ {
            puts(";\n");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstTraceDecl*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstTraceInc*) override {}
    // NOPs
    ਖਾਲੀ ਦੇਖੋ(AstPragma*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstStmtPragma*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCell*) override {}  // Handled outside the Visit class
    // Default
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        ਜੇ (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ EmitVBaseVisitorConst(ਤਾਰਕਿਕ suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = ਮੂਲ;
};

//######################################################################
// Emit to an output file

ਵਰਗ EmitVFileVisitor final : ਜਨਤਕ EmitVBaseVisitorConst {
    // STATE
    V3_ਬਾਹਰਲਾ_VFile& m_ਦਾ;  // The output file
    // METHODS
    ਖਾਲੀ putsNoTracking(ਅਚਲ string& str) override { m_ਦਾ.putsNoTracking(str); }
    ਖਾਲੀ puts(ਅਚਲ string& str) override { m_ਦਾ.puts(str); }
    ਖਾਲੀ putbs(ਅਚਲ string& str) override { m_ਦਾ.putbs(str); }
    ਖਾਲੀ putfs(Ast_ਗੰਢ*, ਅਚਲ string& str) override { putbs(str); }
    ਖਾਲੀ putqs(Ast_ਗੰਢ*, ਅਚਲ string& str) override { putbs(str); }

ਜਨਤਕ:
    EmitVFileVisitor(Ast_ਗੰਢ* nodep, V3_ਬਾਹਰਲਾ_VFile& ਦਾ, ਤਾਰਕਿਕ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_ਦਾ{ਦਾ} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = ਮੂਲ;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

ਵਰਗ EmitVStreamVisitor final : ਜਨਤਕ EmitVBaseVisitorConst {
    // STATE
    V3_ਬਾਹਰਲਾ_ਧਾਰਾ m_os;  // The output stream formatter
    ਅਚਲ ਤਾਰਕਿਕ m_tracking;  // Use line tracking
    // METHODS
    ਖਾਲੀ putsNoTracking(ਅਚਲ string& str) override { m_os.putsNoTracking(str); }
    ਖਾਲੀ puts(ਅਚਲ string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    ਖਾਲੀ putbs(ਅਚਲ string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    ਖਾਲੀ putfs(Ast_ਗੰਢ*, ਅਚਲ string& str) override { putbs(str); }
    ਖਾਲੀ putqs(Ast_ਗੰਢ*, ਅਚਲ string& str) override { putbs(str); }

ਜਨਤਕ:
    EmitVStreamVisitor(ਅਚਲ Ast_ਗੰਢ* nodep, std::ostream& os, ਤਾਰਕਿਕ tracking, ਤਾਰਕਿਕ suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3_ਬਾਹਰਲਾ_Formatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ਗੰਢ*>(nodep));
    }
    ~EmitVStreamVisitor() override = ਮੂਲ;
};

//######################################################################
// EmitV class functions

ਖਾਲੀ V3EmitV::verilog_For_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ਝੂਠ, ਝੂਠ}; }
}

ਖਾਲੀ V3EmitV::debug_Verilog_For_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ ਸੱਚ, ਸੱਚ}; }
}

std::string V3EmitV::debug_Verilog_For_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* nodep) {
    std::stringstream ss;
    debug_Verilog_For_ਰੁੱਖ(nodep, ss);
    ਵਾਪਸ ss.str();
}

ਖਾਲੀ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    ਲਈ (Ast_ਗੰਢ_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ਗੰਢ_File)) {
        AstVFile* ਅਚਲ vfilep = VN_CAST(filep, VFile);
        ਜੇ (vfilep && vfilep->tblockp()) {
            V3_ਬਾਹਰਲਾ_VFile ਦਾ{vfilep->name()};
            ਦਾ.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), ਦਾ, ਝੂਠ};
        }
    }
}

ਖਾਲੀ V3EmitV::debugEmitV(ਅਚਲ string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3_ਬਾਹਰਲਾ_VFile ਦਾ{filename};
    EmitVFileVisitor{v3Global.rootp(), ਦਾ, ਸੱਚ};
}
