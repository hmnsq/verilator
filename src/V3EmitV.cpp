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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3EmitV.h"

#अन्तर्भाव "V3EmitCBase.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

वग्ग EmitVBaseVisitorConst VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    // STATE - across all visitors
    ध्रुव तार्किक m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_वृक्षः* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    तार्किक m_suppressSemi = मिच्छा;  // Non-statement, don't print ;
    तार्किक m_suppressVarSemi = मिच्छा;  // Suppress emitting semicolon for AstVars
    तार्किक m_suppressSampled = मिच्छा;  // Suppress emitting sampled in assertion properties
    तार्किक m_arrayPost = मिच्छा;  // Print array information that goes after identifier (vs after)
    तार्किक m_prefixed = सच्च;  // Whether constants need to be prefixed
    std::deque<Ast_ग्रन्थिः_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    आभास शून्य puts(ध्रुव string& str) = 0;
    आभास शून्य putbs(ध्रुव string& str) = 0;
    आभास शून्य putfs(Ast_ग्रन्थिः* nodep, ध्रुव string& str) = 0;  // Fileline and node %% mark
    आभास शून्य putqs(Ast_ग्रन्थिः* nodep, ध्रुव string& str) = 0;  // Fileline quiet w/o %% mark
    आभास शून्य putsNoTracking(ध्रुव string& str) = 0;
    आभास शून्य putsQuoted(ध्रुव string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    शून्य iterateAndCommaConstNull(Ast_ग्रन्थिः* nodep) {
        अत्थाय (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            सचे (nodep->nextp()) puts(", ");
        }
    }
    शून्य emitPacked() {
        अत्थाय (Ast_ग्रन्थिः_Array_DType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    शून्य emitNodesWithText(Ast_ग्रन्थिः* nodesp, तार्किक tracking, ध्रुव std::string& separator) {
        अत्थाय (Ast_ग्रन्थिः* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstText* ध्रुव textp = VN_CAST(nodep, Text)) {
                सचे (tracking) {
                    puts(textp->text());
                } अञ्ञथा {
                    putsNoTracking(textp->text());
                }
            } अञ्ञथा {
                iterateConst(nodep);
            }
            सचे (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    शून्य visit(AstPort* nodep) override {}
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    शून्य visit(AstGenBlock* nodep) override {
        ध्रुव std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    शून्य visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    शून्य visit(AstGenCaseItem* nodep) override {
        सचे (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } अञ्ञथा {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    शून्य visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = सच्च;
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
    शून्य visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        सचे (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    शून्य visit(AstBegin* nodep) override {
        सचे (nodep->name() == "") {
            putbs("begin\n");
        } अञ्ञथा {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    शून्य visit(AstFork* nodep) override {
        सचे (nodep->name() == "") {
            putbs("fork\n");
        } अञ्ञथा {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    शून्य visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    शून्य visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    शून्य visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstAlways* nodep) override {
        सचे (ध्रुव AstAssignW* ध्रुव ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            सचे (!ap->nextp()) {
                putfs(nodep, "assign ");
                सचे (Ast_ग्रन्थिः* ध्रुव tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                सचे (!m_suppressSemi) puts(";\n");
                पटिनिवत्त;
            }
        }
        putfs(nodep, "always ");
        सचे (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        अञ्ञथा {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        सचे (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        सचे (!m_suppressSemi) puts(";\n");
    }
    शून्य visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        सचे (!m_suppressSemi) puts(";\n");
    }
    शून्य visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        सचे (!m_suppressSemi) puts(";\n");
    }
    शून्य visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        सचे (!m_suppressSemi) puts(";\n");
    }
    शून्य visit(AstBreak*) override {
        putbs("break");
        सचे (!m_suppressSemi) puts(";\n");
    }
    शून्य visit(Ast_Sen_वृक्षः* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        अत्थाय (Ast_ग्रन्थिः* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            सचे (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    शून्य visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        सचे (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        सचे (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    शून्य visit(AstCase* nodep) override {
        putfs(nodep, "");
        सचे (nodep->priorityPragma()) puts("priority ");
        सचे (nodep->uniquePragma()) puts("unique ");
        सचे (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        सचे (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            सचे (nodep->fullPragma()) puts(" full_case");
            सचे (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    शून्य visit(AstCaseItem* nodep) override {
        सचे (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } अञ्ञथा {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstContinue*) override {
        putbs("continue");
        सचे (!m_suppressSemi) puts(";\n");
    }
    शून्य visit(Ast_ग्रन्थिः_Cover_Decl*) override {}  // N/A
    शून्य visit(AstCoverInc*) override {}  // N/A
    शून्य visit(AstCoverToggle*) override {}  // N/A

    शून्य visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        सचे (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    शून्य visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        सचे (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    शून्य emitCoverageOption(Ast_ग्रन्थिः* nodep, तार्किक typeOption, VCoverOptionType optType,
                            Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    शून्य visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    शून्य visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    शून्य visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        सचे (nodep->isArray()) {
            puts("[");
            सचे (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        सचे (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } अञ्ञथा सचे (nodep->transp()) {
            puts(" = ");
            अत्थाय (Ast_ग्रन्थिः* setp = nodep->transp(); setp; setp = setp->nextp()) {
                सचे (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } अञ्ञथा सचे (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } अञ्ञथा सचे (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            अत्थाय (Ast_ग्रन्थिः* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                सचे (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        सचे (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    शून्य visit(AstCoverWith* nodep) override {
        ध्रुव तार्किक rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        सचे (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        सचे (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    शून्य visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        सचे (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        सचे (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    शून्य visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        सचे (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    शून्य visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    शून्य visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    शून्य visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        सचे (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        सचे (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    शून्य visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    शून्य visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            सचे (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        सचे (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        सचे (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } अञ्ञथा {
            puts(";\n");
        }
    }
    शून्य visit(AstCoverTransSet* nodep) override {
        puts("(");
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            सचे (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    शून्य visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    शून्य visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        सचे (AstConst* ध्रुव lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->अङ्कः().toString());
        } अञ्ञथा {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    शून्य visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    शून्य visit(Ast_मानम्_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    शून्य visit_ग्रन्थिः_Display(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* fileOrStrgp, ध्रुव string& text,
                          Ast_ग्रन्थिः* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        सचे (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        अत्थाय (Ast_ग्रन्थिः* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    शून्य visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    शून्य visit(AstDisplay* nodep) override {
        visit_ग्रन्थिः_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य visit(AstElabDisplay* nodep) override {
        visit_ग्रन्थिः_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य visit(AstFScanF* nodep) override {
        visit_ग्रन्थिः_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    शून्य visit(AstSScanF* nodep) override {
        visit_ग्रन्थिः_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    शून्य visit(AstSFormat* nodep) override {
        visit_ग्रन्थिः_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    शून्य visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    शून्य visit(AstSFormatF* nodep) override {
        visit_ग्रन्थिः_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    शून्य visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    शून्य visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    शून्य visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        सचे (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    शून्य visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        सचे (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // Allocate label number
        ध्रुव size_t n = m_labelNumbers.size();
        ध्रुव तार्किक newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    शून्य visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        ध्रुव auto it = m_labelNumbers.find(nodep->blockp());
        ध्रुव std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    शून्य visit(Ast_ग्रन्थिः_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        सचे (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        सचे (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        सचे (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        सचे (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    शून्य visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    शून्य visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    शून्य visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        सचे (AstLoopTest* ध्रुव testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            पटिनिवत्त;
        }
        // Special case when the AstLoopTest is last for output readability
        सचे (Ast_ग्रन्थिः* lastp = nodep->stmtsp()) {
            याव (Ast_ग्रन्थिः* ध्रुव nextp = lastp->nextp()) lastp = nextp;
            सचे (AstLoopTest* ध्रुव testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                अत्थाय (Ast_ग्रन्थिः* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                पटिनिवत्त;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    शून्य visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        putfs(nodep, "");
        सचे (ध्रुव AstIf* ध्रुव ifp = VN_CAST(nodep, If)) {
            सचे (ifp->priorityPragma()) puts("priority ");
            सचे (ifp->uniquePragma()) puts("unique ");
            सचे (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        सचे (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    शून्य visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        सचे (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            सचे (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    शून्य visit(AstSampled* nodep) override {
        सचे (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    शून्य visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    शून्य visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        सचे (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    शून्य visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        सचे (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    शून्य visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    शून्य visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    शून्य visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        सचे (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    शून्य visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    शून्य visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    शून्य visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    शून्य visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    शून्य visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    शून्य visit(Ast_Stmt_व्यञ्जकम्* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    शून्य visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    शून्य visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), सच्च, nodep->separator());
        puts(nodep->suffix());
    }
    शून्य visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), सच्च, "");
        puts(");\n");
    }
    शून्य visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), सच्च, "");
        puts(")");
    }
    शून्य visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), मिच्छा, "");
        puts(");\n");
    }
    शून्य visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), मिच्छा, "");
        puts(")");
    }

    शून्य visit(AstScopeName* nodep) override {}
    शून्य visit(Ast_व्यञ्जकम्_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    शून्य visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    शून्य visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    आभास शून्य emitVerilogFormat(Ast_ग्रन्थिः* nodep, ध्रुव string& format, Ast_ग्रन्थिः* lhsp = nullptr,
                                   Ast_ग्रन्थिः* ध्रुव rhsp = nullptr, Ast_ग्रन्थिः* thsp = nullptr,
                                   Ast_ग्रन्थिः* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        तार्किक inPct = मिच्छा;
        putbs("");
        अत्थाय (ध्रुव वर्ण c : format) {
            सचे (!inPct && c == '%') {
                inPct = सच्च;
            } अञ्ञथा सचे (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } अञ्ञथा {  // Format character
                inPct = मिच्छा;
                विकल्प (c) {
                पक्ष '%': puts("%"); भङ्ग;
                पक्ष 'f': putfs(nodep, ""); भङ्ग;
                पक्ष 'k': putbs(""); भङ्ग;
                पक्ष 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    भङ्ग;
                }
                पक्ष 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    भङ्ग;
                }
                पक्ष 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    भङ्ग;
                }
                पक्ष 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    भङ्ग;
                }
                पक्ष 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    भङ्ग;
                }
                स्वभाव: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); भङ्ग;
                }
            }
        }
    }

    शून्य visit(Ast_ग्रन्थिः_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    शून्य visit(Ast_ग्रन्थिः_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    शून्य visit(Ast_ग्रन्थिः_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    शून्य visit(Ast_ग्रन्थिः_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    शून्य visit(Ast_ग्रन्थिः_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    शून्य visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        सचे (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    शून्य visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        अत्थाय (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            सचे (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    शून्य visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    शून्य visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    शून्य visit(AstRange* nodep) override {
        puts("[");
        सचे (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->वामः_Const()));
            puts(":");
            puts(cvtToStr(nodep->दक्षिणः_Const()));
        } अञ्ञथा {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    शून्य visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    शून्य visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        पूर्णाङ्क offset = 0;
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->fromp()->dtypep();
        सचे (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ध्रुव basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        सचे (VN_IS(nodep->lsbp(), Const)) {
            सचे (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } अञ्ञथा {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } अञ्ञथा {
            iterateAndNextConstNull(nodep->lsbp());
            सचे (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    शून्य visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    शून्य visit(AstThisRef* /*nodep*/) override { puts("this"); }
    शून्य visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    शून्य visit(Ast_ग्रन्थिः_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        सचे (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } अञ्ञथा सचे (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } अञ्ञथा सचे (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        सचे (AstSampled* ध्रुव sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } अञ्ञथा {
            iterateAndNextConstNull(nodep->propp());
        }
        सचे (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } अञ्ञथा {
            puts(");\n");
        }

        सचे (ध्रुव AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } अञ्ञथा सचे (ध्रुव AstAssertIntrinsic* ध्रुव assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    शून्य visit(AstAssocArrayDType* nodep) override {
        सचे (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अञ्ञथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = मिच्छा;
            puts("[");
            iterateConst(nodep->कुञ्चिका_DTypep());
            puts("]");
            m_arrayPost = सच्च;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(AstBasicDType* nodep) override {
        सचे (m_arrayPost) पटिनिवत्त;
        putfs(nodep, nodep->prettyName());
        सचे (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        सचे (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) पटिनिवत्त;
        emitPacked();
        सचे (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } अञ्ञथा सचे (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->वामः()));
            puts(":");
            puts(cvtToStr(nodep->दक्षिणः()));
            puts("] ");
        }
    }
    शून्य visit(AstConstDType* nodep) override {
        सचे (m_arrayPost) पटिनिवत्त;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    शून्य visit(AstDynArrayDType* nodep) override {
        सचे (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अञ्ञथा {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(AstEnumDType* nodep) override {
        सचे (m_arrayPost) पटिनिवत्त;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        सचे (Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    शून्य visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        सचे (nodep->nextp()) puts(",");
        puts("\n");
    }
    शून्य visit(Ast_ग्रन्थिः_Array_DType* nodep) override {
        सचे (!m_arrayPost) {
            सचे (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.प्रक्षिप_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } अञ्ञथा {
            सचे (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = मिच्छा;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = सच्च;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        सचे (m_arrayPost) {
            puts(" (");
            सचे (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } अञ्ञथा {
                puts("????");
            }
            puts(")");
            पटिनिवत्त;
        }
        puts(nodep->ifaceName());
    }
    शून्य visit(AstRefDType* nodep) override {
        सचे (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } अञ्ञथा {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    शून्य visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    शून्य visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    शून्य visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        सचे (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } अञ्ञथा {
            puts("????");
        }
        puts(");\n");
    }
    शून्य visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        सचे (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = सच्च;
            iterateConst(nodep->varp());
        } अञ्ञथा {
            puts(nodep->prettyName());
        }
        सचे (nodep->nextp()) puts(", ");
    }
    शून्य visit(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        सचे (m_arrayPost) पटिनिवत्त;
        puts(nodep->verilogKwd() + " ");
        सचे (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            अत्थाय (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    शून्य visit(AstMemberDType* nodep) override {
        सचे (m_arrayPost) पटिनिवत्त;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    शून्य visit(AstQueueDType* nodep) override {
        सचे (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अञ्ञथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = मिच्छा;
            puts("[$");
            सचे (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = सच्च;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } अञ्ञथा {
            putfs(nodep, nodep->prettyName());
        }
        सचे (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    शून्य visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    शून्य visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    शून्य visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    शून्य visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    शून्य visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    शून्य visit(AstPropSpec* nodep) override {
        सचे (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            अत्थाय (Ast_ग्रन्थिः* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                सचे (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        सचे (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        सचे (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        सचे (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    शून्य visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    शून्य visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    शून्य visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    शून्य visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        सचे (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    शून्य visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = सच्च;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    शून्य visit(AstVarRef* nodep) override {
        सचे (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } अञ्ञथा {
            सचे (nodep->varp()) {
                सचे (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } अञ्ञथा {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } अञ्ञथा {
                putfs(nodep, nodep->name());
            }
        }
    }
    शून्य visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        सचे (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } अञ्ञथा {
            puts(nodep->prettyName());
        }
    }
    शून्य visit(AstConst* nodep) override { putfs(nodep, nodep->अङ्कः().ascii(m_prefixed, सच्च)); }
    शून्य visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    शून्य visit(Ast_शिखरम्_Scope* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstVar* nodep) override {
        सचे (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = मिच्छा;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = सच्च;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    शून्य visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    शून्य visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        सचे (nodep->isCycleDelay()) {
            puts("##");
        } अञ्ञथा {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = मिच्छा;
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } अञ्ञथा {
            iterateConst(nodep->lhsp());
        }
        सचे (!m_suppressSemi) {
            puts(";\n");
        } अञ्ञथा {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    शून्य visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        सचे (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } अञ्ञथा {
            puts(";\n");
        }
    }
    शून्य visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    शून्य visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    शून्य visit(AstVarScope*) override {}
    शून्य visit(AstTraceDecl*) override {}
    शून्य visit(AstTraceInc*) override {}
    // NOPs
    शून्य visit(AstPragma*) override {}
    शून्य visit(AstStmtPragma*) override {}
    शून्य visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        सचे (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

सार्वजनिक:
    explicit EmitVBaseVisitorConst(तार्किक suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = स्वभाव;
};

//######################################################################
// Emit to an output file

वग्ग EmitVFileVisitor final : सार्वजनिक EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    शून्य putsNoTracking(ध्रुव string& str) override { m_of.putsNoTracking(str); }
    शून्य puts(ध्रुव string& str) override { m_of.puts(str); }
    शून्य putbs(ध्रुव string& str) override { m_of.putbs(str); }
    शून्य putfs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }
    शून्य putqs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }

सार्वजनिक:
    EmitVFileVisitor(Ast_ग्रन्थिः* nodep, V3OutVFile& of, तार्किक suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = स्वभाव;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

वग्ग EmitVStreamVisitor final : सार्वजनिक EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    ध्रुव तार्किक m_tracking;  // Use line tracking
    // METHODS
    शून्य putsNoTracking(ध्रुव string& str) override { m_os.putsNoTracking(str); }
    शून्य puts(ध्रुव string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    शून्य putbs(ध्रुव string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    शून्य putfs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }
    शून्य putqs(Ast_ग्रन्थिः*, ध्रुव string& str) override { putbs(str); }

सार्वजनिक:
    EmitVStreamVisitor(ध्रुव Ast_ग्रन्थिः* nodep, std::ostream& os, तार्किक tracking, तार्किक suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_ग्रन्थिः*>(nodep));
    }
    ~EmitVStreamVisitor() override = स्वभाव;
};

//######################################################################
// EmitV class functions

शून्य V3EmitV::verilog_For_वृक्षः(ध्रुव Ast_ग्रन्थिः* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ मिच्छा, मिच्छा}; }
}

शून्य V3EmitV::debug_Verilog_For_वृक्षः(ध्रुव Ast_ग्रन्थिः* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ सच्च, सच्च}; }
}

std::string V3EmitV::debug_Verilog_For_वृक्षः(ध्रुव Ast_ग्रन्थिः* nodep) {
    std::stringstream ss;
    debug_Verilog_For_वृक्षः(nodep, ss);
    पटिनिवत्त ss.str();
}

शून्य V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    अत्थाय (Ast_ग्रन्थिः_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), ग्रन्थिः_File)) {
        AstVFile* ध्रुव vfilep = VN_CAST(filep, VFile);
        सचे (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, मिच्छा};
        }
    }
}

शून्य V3EmitV::debugEmitV(ध्रुव string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, सच्च};
}
