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

वर्ग EmitVBaseVisitorConst VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    // STATE - across all visitors
    स्थिरांक तार्किक m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    Ast_Sen_वृक्ष* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    तार्किक m_suppressSemi = असत्य;  // Non-statement, don't print ;
    तार्किक m_suppressVarSemi = असत्य;  // Suppress emitting semicolon for AstVars
    तार्किक m_suppressSampled = असत्य;  // Suppress emitting sampled in assertion properties
    तार्किक m_arrayPost = असत्य;  // Print array information that goes after identifier (vs after)
    तार्किक m_prefixed = सत्य;  // Whether constants need to be prefixed
    std::deque<Ast_गाठ_Array_DType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, माप_प्रकार> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    आभासी शून्य माला_दो(स्थिरांक string& str) = 0;
    आभासी शून्य putbs(स्थिरांक string& str) = 0;
    आभासी शून्य putfs(Ast_गाठ* nodep, स्थिरांक string& str) = 0;  // Fileline and node %% mark
    आभासी शून्य putqs(Ast_गाठ* nodep, स्थिरांक string& str) = 0;  // Fileline quiet w/o %% mark
    आभासी शून्य putsNoTracking(स्थिरांक string& str) = 0;
    आभासी शून्य putsQuoted(स्थिरांक string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    शून्य iterateAndCommaConstNull(Ast_गाठ* nodep) {
        साठी (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            जर (nodep->nextp()) माला_दो(", ");
        }
    }
    शून्य emitPacked() {
        साठी (Ast_गाठ_Array_DType* packedp : m_packedps) {
            माला_दो(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    शून्य emitNodesWithText(Ast_गाठ* nodesp, तार्किक tracking, स्थिरांक std::string& separator) {
        साठी (Ast_गाठ* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            जर (स्थिरांक AstText* स्थिरांक textp = VN_CAST(nodep, Text)) {
                जर (tracking) {
                    माला_दो(textp->text());
                } अन्यथा {
                    putsNoTracking(textp->text());
                }
            } अन्यथा {
                iterateConst(nodep);
            }
            जर (nodep->nextp()) माला_दो(separator);
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    शून्य visit(AstPort* nodep) override {}
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        माला_दो(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    शून्य visit(AstGenBlock* nodep) override {
        स्थिरांक std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        माला_दो("end" + name + '\n');
    }
    शून्य visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        माला_दो(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    शून्य visit(AstGenCaseItem* nodep) override {
        जर (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } अन्यथा {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    शून्य visit(AstGenFor* nodep) override {
        putfs(nodep, "/* generate */ for (");
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = सत्य;
            iterateAndNextConstNull(nodep->initsp());
            माला_दो(";");
            iterateAndNextConstNull(nodep->condp());
            माला_दो(";");
            iterateAndNextConstNull(nodep->incsp());
        }
        माला_दो(") begin\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "end\n");
    }
    शून्य visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        माला_दो("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        माला_दो(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        जर (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    शून्य visit(AstBegin* nodep) override {
        जर (nodep->name() == "") {
            putbs("begin\n");
        } अन्यथा {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        माला_दो("end\n");
    }
    शून्य visit(AstFork* nodep) override {
        जर (nodep->name() == "") {
            putbs("fork\n");
        } अन्यथा {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        माला_दो(nodep->joinType().verilogKwd());
        माला_दो("\n");
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
        जर (स्थिरांक AstAssignW* स्थिरांक ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            जर (!ap->nextp()) {
                putfs(nodep, "assign ");
                जर (Ast_गाठ* स्थिरांक tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                जर (!m_suppressSemi) माला_दो(";\n");
                परत;
            }
        }
        putfs(nodep, "always ");
        जर (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        अन्यथा {
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
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (VN_IS(nodep, AssignForce)) माला_दो("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        जर (!m_suppressSemi) माला_दो(";\n");
    }
    शून्य visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        साठी (Ast_गाठ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        जर (!m_suppressSemi) माला_दो(";\n");
    }
    शून्य visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        जर (!m_suppressSemi) माला_दो(";\n");
    }
    शून्य visit(AstRelease* nodep) override {
        माला_दो("release ");
        iterateAndNextConstNull(nodep->lhsp());
        जर (!m_suppressSemi) माला_दो(";\n");
    }
    शून्य visit(AstBreak*) override {
        putbs("break");
        जर (!m_suppressSemi) माला_दो(";\n");
    }
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        साठी (Ast_गाठ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            जर (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        माला_दो(")");
    }
    शून्य visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        जर (nodep->edgeType() != VEdgeType::ET_CHANGED) माला_दो(nodep->edgeType().verilogKwd());
        जर (nodep->sensp()) माला_दो(" ");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCReset* /*nodep*/) override { माला_दो("/*CRESET*/"); }
    शून्य visit(AstCase* nodep) override {
        putfs(nodep, "");
        जर (nodep->priorityPragma()) माला_दो("priority ");
        जर (nodep->uniquePragma()) माला_दो("unique ");
        जर (nodep->unique0Pragma()) माला_दो("unique0 ");
        माला_दो(nodep->verilogKwd());
        माला_दो(" (");
        iterateAndNextConstNull(nodep->exprp());
        माला_दो(")\n");
        जर (nodep->fullPragma() || nodep->parallelPragma()) {
            माला_दो(" // synopsys");
            जर (nodep->fullPragma()) माला_दो(" full_case");
            जर (nodep->parallelPragma()) माला_दो(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    शून्य visit(AstCaseItem* nodep) override {
        जर (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } अन्यथा {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    शून्य visit(AstComment* nodep) override {
        माला_दो("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstContinue*) override {
        putbs("continue");
        जर (!m_suppressSemi) माला_दो(";\n");
    }
    शून्य visit(Ast_गाठ_Cover_Decl*) override {}  // N/A
    शून्य visit(AstCoverInc*) override {}  // N/A
    शून्य visit(AstCoverToggle*) override {}  // N/A

    शून्य visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        जर (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    शून्य visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        जर (nodep->binsp() || nodep->optionsp()) {
            माला_दो(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            माला_दो("}");
        }
        माला_दो(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    शून्य emitCoverageOption(Ast_गाठ* nodep, तार्किक typeOption, VCoverOptionType optType,
                            Ast_गाठ_अभिव्यक्ती* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        माला_दो(";\n");
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
        माला_दो(nodep->name());
        जर (nodep->isArray()) {
            माला_दो("[");
            जर (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            माला_दो("]");
        }
        जर (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            माला_दो(" = default");
        } अन्यथा जर (nodep->transp()) {
            माला_दो(" = ");
            साठी (Ast_गाठ* setp = nodep->transp(); setp; setp = setp->nextp()) {
                जर (setp != nodep->transp()) माला_दो(", ");
                iterateConst(setp);
            }
        } अन्यथा जर (VN_IS(nodep->rangesp(), CoverWith)) {
            माला_दो(" = ");
            iterateConst(nodep->rangesp());
        } अन्यथा जर (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            माला_दो(" = {");
            साठी (Ast_गाठ* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                जर (rangep != nodep->rangesp()) माला_दो(", ");
                iterateConst(rangep);
            }
            माला_दो("}");
        }
        जर (nodep->iffp()) {
            माला_दो(" iff (");
            iterateConst(nodep->iffp());
            माला_दो(")");
        }
        माला_दो(";\n");
    }
    शून्य visit(AstCoverWith* nodep) override {
        स्थिरांक तार्किक rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        जर (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        जर (rangeList) माला_दो("}");
        माला_दो(" with (");
        iterateConst(nodep->filterp());
        माला_दो(")");
    }
    शून्य visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        जर (!nodep->name().empty()) माला_दो("." + nodep->name());
        माला_दो(")");
        जर (nodep->rangesp()) {
            माला_दो(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            माला_दो("}");
        }
    }
    शून्य visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        जर (nodep->iffp()) {
            माला_दो(" iff (");
            iterateConst(nodep->iffp());
            माला_दो(")");
        }
        माला_दो(";\n");
    }
    शून्य visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    शून्य visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        माला_दो(")");
    }
    शून्य visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        जर (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        माला_दो(")");
        जर (nodep->countp()) {
            माला_दो(" matches ");
            iterateConst(nodep->countp());
            माला_दो(")");
        }
    }
    शून्य visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    शून्य visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        साठी (Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            जर (itemp != nodep->itemsp()) माला_दो(", ");
            iterateConst(itemp);
        }
        जर (nodep->iffp()) {
            माला_दो(" iff (");
            iterateConst(nodep->iffp());
            माला_दो(")");
        }
        जर (nodep->binsp() || nodep->optionsp()) {
            माला_दो(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            माला_दो("}\n");
        } अन्यथा {
            माला_दो(";\n");
        }
    }
    शून्य visit(AstCoverTransSet* nodep) override {
        माला_दो("(");
        साठी (Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            जर (itemp != nodep->itemsp()) माला_दो(" => ");
            iterateConst(itemp);
        }
        माला_दो(")");
    }
    शून्य visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    शून्य visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        जर (AstConst* स्थिरांक lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->अंक().toString());
        } अन्यथा {
            माला_दो("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            माला_दो(")");
        }
    }
    शून्य visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        माला_दो(")");
    }
    शून्य visit(Ast_मूल्य_Plus_Args* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        माला_दो(")");
    }
    शून्य visit_गाठ_Display(Ast_गाठ* nodep, Ast_गाठ* fileOrStrgp, स्थिरांक string& text,
                          Ast_गाठ* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        जर (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        साठी (Ast_गाठ* expp = exprsp; expp; expp = expp->nextp()) {
            माला_दो(", ");
            iterateConstNull(expp);
        }
        माला_दो(");\n");
    }
    शून्य visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    शून्य visit(AstDisplay* nodep) override {
        visit_गाठ_Display(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य visit(AstElabDisplay* nodep) override {
        visit_गाठ_Display(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य visit(AstFScanF* nodep) override {
        visit_गाठ_Display(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    शून्य visit(AstSScanF* nodep) override {
        visit_गाठ_Display(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    शून्य visit(AstSFormat* nodep) override {
        visit_गाठ_Display(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    शून्य visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    शून्य visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    शून्य visit(AstSFormatF* nodep) override {
        visit_गाठ_Display(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    शून्य visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        माला_दो(");\n");
    }
    शून्य visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        माला_दो(");\n");
    }
    शून्य visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        जर (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        माला_दो(");\n");
    }
    शून्य visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        जर (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        माला_दो(");\n");
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // Allocate label number
        स्थिरांक माप_प्रकार n = m_labelNumbers.size();
        स्थिरांक तार्किक newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        माला_दो("end\n");
    }
    शून्य visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        स्थिरांक स्वतः it = m_labelNumbers.find(nodep->blockp());
        स्थिरांक std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    शून्य visit(Ast_गाठ_Read_Write_Mem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        जर (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        जर (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        जर (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        जर (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        माला_दो(");\n");
    }
    शून्य visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        माला_दो(");\n");
    }
    शून्य visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        माला_दो(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    शून्य visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        जर (AstLoopTest* स्थिरांक testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            माला_दो(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            माला_दो("end\n");
            परत;
        }
        // Special case when the AstLoopTest is last for output readability
        जर (Ast_गाठ* lastp = nodep->stmtsp()) {
            पर्यंत (Ast_गाठ* स्थिरांक nextp = lastp->nextp()) lastp = nextp;
            जर (AstLoopTest* स्थिरांक testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                साठी (Ast_गाठ* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                माला_दो("end while (");
                iterateConst(testp->condp());
                माला_दो(")\n");
                परत;
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
        माला_दो(")) break;\n");
    }
    शून्य visit(Ast_गाठ_If* nodep) override {
        putfs(nodep, "");
        जर (स्थिरांक AstIf* स्थिरांक ifp = VN_CAST(nodep, If)) {
            जर (ifp->priorityPragma()) माला_दो("priority ");
            जर (ifp->uniquePragma()) माला_दो("unique ");
            जर (ifp->unique0Pragma()) माला_दो("unique0 ");
        }
        माला_दो("if (");
        iterateAndNextConstNull(nodep->condp());
        माला_दो(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        जर (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    शून्य visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        जर (nodep->ticksp() || nodep->sentreep()) {
            माला_दो(", ");
            iterateAndNextConstNull(nodep->ticksp());
            जर (nodep->sentreep()) {
                माला_दो(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        माला_दो(")");
    }
    शून्य visit(AstSampled* nodep) override {
        जर (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            माला_दो(")");
        }
    }
    शून्य visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        माला_दो(")");
    }
    शून्य visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        जर (nodep->sentreep()) {
            माला_दो(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        माला_दो(")");
    }
    शून्य visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        जर (nodep->sentreep()) {
            माला_दो(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        माला_दो(")");
    }
    शून्य visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        माला_दो(")");
    }
    शून्य visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        माला_दो(")");
    }
    शून्य visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        जर (nodep->sentreep()) {
            माला_दो(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        माला_दो(")");
    }
    शून्य visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        माला_दो(")");
    }
    शून्य visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(";\n");
    }
    शून्य visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        माला_दो(";\n");
    }
    शून्य visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    शून्य visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        iterateConst(nodep->exprp());
        माला_दो(";\n");
    }

    // Nodes involing AstText
    शून्य visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    शून्य visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        माला_दो(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), सत्य, nodep->separator());
        माला_दो(nodep->suffix());
    }
    शून्य visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), सत्य, "");
        माला_दो(");\n");
    }
    शून्य visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), सत्य, "");
        माला_दो(")");
    }
    शून्य visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), असत्य, "");
        माला_दो(");\n");
    }
    शून्य visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), असत्य, "");
        माला_दो(")");
    }

    शून्य visit(AstScopeName* nodep) override {}
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        माला_दो(");\n");
    }

    शून्य visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        माला_दो(")");
        iterateConstNull(nodep->withp());
    }
    शून्य visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        माला_दो(")");
    }

    // Operators
    आभासी शून्य emitVerilogFormat(Ast_गाठ* nodep, स्थिरांक string& format, Ast_गाठ* lhsp = nullptr,
                                   Ast_गाठ* स्थिरांक rhsp = nullptr, Ast_गाठ* thsp = nullptr,
                                   Ast_गाठ* fhsp = nullptr) {
        // Look at emitVerilog() format for term/uni/dual/triops,
        // and write out appropriate text.
        //      %f      Potential fileline-if-change and line break
        //      %l      lhsp - if appropriate
        //      %r      rhsp - if appropriate
        //      %t      thsp - if appropriate
        //      %o      fhsp - if appropriate
        //      %d      dtypep - if appropriate
        //      %k      Potential line break
        तार्किक inPct = असत्य;
        putbs("");
        साठी (स्थिरांक अक्षर c : format) {
            जर (!inPct && c == '%') {
                inPct = सत्य;
            } अन्यथा जर (!inPct) {  // Normal text
                string s;
                s += c;
                माला_दो(s);
            } अन्यथा {  // Format character
                inPct = असत्य;
                निवड (c) {
                स्थिती '%': माला_दो("%"); थांबा;
                स्थिती 'f': putfs(nodep, ""); थांबा;
                स्थिती 'k': putbs(""); थांबा;
                स्थिती 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    थांबा;
                }
                स्थिती 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    थांबा;
                }
                स्थिती 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    थांबा;
                }
                स्थिती 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    थांबा;
                }
                स्थिती 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    थांबा;
                }
                पूर्वनिर्धारित: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); थांबा;
                }
            }
        }
    }

    शून्य visit(Ast_गाठ_Termop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    शून्य visit(Ast_गाठ_Uniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    शून्य visit(Ast_गाठ_Biop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    शून्य visit(Ast_गाठ_Triop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    शून्य visit(Ast_गाठ_Quadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो(".");
        माला_दो(nodep->prettyName());
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        माला_दो(".");
        माला_दो(nodep->prettyName());
    }
    शून्य visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        जर (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        माला_दो(")");
    }
    शून्य visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        साठी (स्थिरांक AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            जर (itemp != nodep->itemsp()) putbs(", ");
            माला_दो(cvtToStr(itemp->index()));
            माला_दो(":");
            iterateConst(itemp->valuep());
        }
        माला_दो("}");
    }
    शून्य visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        माला_दो(")");
    }
    शून्य visit(AstInsideRange* nodep) override {
        माला_दो("[");
        iterateAndNextConstNull(nodep->lhsp());
        माला_दो(":");
        iterateAndNextConstNull(nodep->rhsp());
        माला_दो("]");
    }
    शून्य visit(AstRange* nodep) override {
        माला_दो("[");
        जर (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            माला_दो(cvtToStr(nodep->डावा_Const()));
            माला_दो(":");
            माला_दो(cvtToStr(nodep->उजवा_Const()));
        } अन्यथा {
            iterateAndNextConstNull(nodep->leftp());
            माला_दो(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        माला_दो("]");
    }
    शून्य visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    शून्य visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        पूर्णांक offset = 0;
        Ast_गाठ_DType* स्थिरांक dtypep = nodep->fromp()->dtypep();
        जर (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* स्थिरांक basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        माला_दो("[");
        जर (VN_IS(nodep->lsbp(), Const)) {
            जर (nodep->widthConst() == 1) {
                माला_दो(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } अन्यथा {
                माला_दो(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                माला_दो(":");
                माला_दो(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } अन्यथा {
            iterateAndNextConstNull(nodep->lsbp());
            जर (offset != 0) {
                माला_दो(" + ");
                माला_दो(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            माला_दो(cvtToStr(nodep->widthConst()));
        }
        माला_दो("]");
    }
    शून्य visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        माला_दो(cvtToStr(nodep->declRange()));
    }
    शून्य visit(AstThisRef* /*nodep*/) override { माला_दो("this"); }
    शून्य visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        माला_दो(";\n");
    }
    शून्य visit(Ast_गाठ_Cover_Or_Assert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        जर (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            माला_दो("#0 ");
        } अन्यथा जर (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            माला_दो("final ");
        } अन्यथा जर (nodep->userType() == VAssertType::CONCURRENT) {
            माला_दो("property ");
        }
        iterateConstNull(nodep->sentreep());
        माला_दो("(");
        जर (AstSampled* स्थिरांक sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } अन्यथा {
            iterateAndNextConstNull(nodep->propp());
        }
        जर (!VN_IS(nodep, Restrict)) {
            माला_दो(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            माला_दो("end\n");
        } अन्यथा {
            माला_दो(");\n");
        }

        जर (स्थिरांक AstAssert* स्थिरांक assertp = VN_CAST(nodep, Assert)) {
            माला_दो("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            माला_दो("end\n");
        } अन्यथा जर (स्थिरांक AstAssertIntrinsic* स्थिरांक assertp = VN_CAST(nodep, AssertIntrinsic)) {
            माला_दो("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            माला_दो("end\n");
        }
    }
    शून्य visit(AstAssocArrayDType* nodep) override {
        जर (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = असत्य;
            माला_दो("[");
            iterateConst(nodep->किल्ली_DTypep());
            माला_दो("]");
            m_arrayPost = सत्य;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(AstBasicDType* nodep) override {
        जर (m_arrayPost) परत;
        putfs(nodep, nodep->prettyName());
        जर (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        जर (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) परत;
        emitPacked();
        जर (nodep->rangep()) {
            माला_दो(" ");
            iterateAndNextConstNull(nodep->rangep());
            माला_दो(" ");
        } अन्यथा जर (nodep->isRanged()) {
            माला_दो(" [");
            माला_दो(cvtToStr(nodep->डावा()));
            माला_दो(":");
            माला_दो(cvtToStr(nodep->उजवा()));
            माला_दो("] ");
        }
    }
    शून्य visit(AstConstDType* nodep) override {
        जर (m_arrayPost) परत;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    शून्य visit(AstDynArrayDType* nodep) override {
        जर (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            माला_दो("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(AstEnumDType* nodep) override {
        जर (m_arrayPost) परत;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        माला_दो("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        माला_दो("}");
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        जर (Ast_गाठ_Module* स्थिरांक classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            माला_दो("::");
        }
        putfs(nodep, nodep->name());
    }
    शून्य visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        माला_दो(" = ");
        iterateConstNull(nodep->valuep());
        जर (nodep->nextp()) माला_दो(",");
        माला_दो("\n");
    }
    शून्य visit(Ast_गाठ_Array_DType* nodep) override {
        जर (!m_arrayPost) {
            जर (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.ढकला_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            जर (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = असत्य;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = सत्य;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        जर (m_arrayPost) {
            माला_दो(" (");
            जर (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } अन्यथा {
                माला_दो("????");
            }
            माला_दो(")");
            परत;
        }
        माला_दो(nodep->ifaceName());
    }
    शून्य visit(AstRefDType* nodep) override {
        जर (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } अन्यथा {
            माला_दो("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    शून्य visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    शून्य visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    शून्य visit(AstModport* nodep) override {
        माला_दो(nodep->verilogKwd());
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        माला_दो(" (\n");
        जर (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } अन्यथा {
            माला_दो("????");
        }
        माला_दो(");\n");
    }
    शून्य visit(AstModportVarRef* nodep) override {
        माला_दो(nodep->direction().verilogKwd());
        माला_दो(" ");
        जर (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = सत्य;
            iterateConst(nodep->varp());
        } अन्यथा {
            माला_दो(nodep->prettyName());
        }
        जर (nodep->nextp()) माला_दो(", ");
    }
    शून्य visit(Ast_गाठ_UOr_Struct_DType* nodep) override {
        जर (m_arrayPost) परत;
        माला_दो(nodep->verilogKwd() + " ");
        जर (nodep->packed()) माला_दो("packed ");
        {
            माला_दो("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            साठी (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            माला_दो("}");
        }
        emitPacked();
    }
    शून्य visit(AstMemberDType* nodep) override {
        जर (m_arrayPost) परत;
        iterateConst(nodep->subDTypep());
        माला_दो(" ");
        माला_दो(nodep->name());
        माला_दो(";\n");
    }
    शून्य visit(AstQueueDType* nodep) override {
        जर (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } अन्यथा {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = असत्य;
            माला_दो("[$");
            जर (nodep->boundp()) {
                माला_दो(":");
                iterateConst(nodep->boundp());
            }
            माला_दो("]");
            m_arrayPost = सत्य;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        जर (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            माला_दो(".");
            माला_दो(nodep->prettyName());
        } अन्यथा {
            putfs(nodep, nodep->prettyName());
        }
        जर (!VN_IS(nodep->taskp(), Property)) {
            माला_दो("(");
            iterateAndNextConstNull(nodep->argsp());
            माला_दो(")");
            iterateConstNull(nodep->withp());
        }
    }
    शून्य visit(AstCCall* nodep) override {
        माला_दो(nodep->funcp()->name());
        माला_दो("(");
        iterateAndNextConstNull(nodep->argsp());
        माला_दो(")");
    }
    शून्य visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    शून्य visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        माला_दो(") ");
    }
    शून्य visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    शून्य visit(AstPrintTimeScale* nodep) override {
        माला_दो(nodep->verilogKwd());
        माला_दो(";\n");
    }
    शून्य visit(AstPropSpec* nodep) override {
        जर (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            साठी (Ast_गाठ* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                जर (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            माला_दो(")");
        }
        जर (nodep->disablep()) {
            माला_दो(" disable iff ");
            iterateConst(nodep->disablep());
        }

        माला_दो(" ");
        जर (nodep->propStrength() != VPropStrength::DEFAULT) {
            माला_दो(nodep->propStrength().ascii());
            माला_दो("(");
        }
        iterateConstNull(nodep->propp());
        जर (nodep->propStrength() != VPropStrength::DEFAULT) माला_दो(")");
        माला_दो("\n");
    }
    शून्य visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    शून्य visit(AstAbortOn* nodep) override {
        माला_दो(nodep->kind().ascii());
        माला_दो(" (");
        iterateConst(nodep->condp());
        माला_दो(") ");
        iterateConst(nodep->propp());
    }
    शून्य visit(AstSClocked* nodep) override {
        माला_दो("@(");
        iterateConst(nodep->sensesp());
        माला_दो(") ");
        iterateConst(nodep->exprp());
        माला_दो("\n");
    }
    शून्य visit(AstPropAlways* nodep) override {
        माला_दो(nodep->isStrong() ? "s_always" : "always");
        जर (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            माला_दो(" [");
            iterateConst(nodep->loBoundp());
            माला_दो(":");
            iterateConst(nodep->hiBoundp());
            माला_दो("]");
        }
        माला_दो(" ");
        iterateConst(nodep->propp());
    }
    शून्य visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = सत्य;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    शून्य visit(AstVarRef* nodep) override {
        जर (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } अन्यथा {
            जर (nodep->varp()) {
                जर (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } अन्यथा {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    माला_दो(nodep->varp()->prettyName());
                }
            } अन्यथा {
                putfs(nodep, nodep->name());
            }
        }
    }
    शून्य visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        माला_दो(".");
        जर (nodep->varp()) {
            माला_दो(nodep->varp()->prettyName());
        } अन्यथा {
            माला_दो(nodep->prettyName());
        }
    }
    शून्य visit(AstConst* nodep) override { putfs(nodep, nodep->अंक().ascii(m_prefixed, सत्य)); }
    शून्य visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    शून्य visit(Ast_शीर्ष_Scope* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    शून्य visit(AstVar* nodep) override {
        जर (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            माला_दो(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = असत्य;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        माला_दो(" ");
        माला_दो(nodep->prettyName());
        m_arrayPost = सत्य;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        माला_दो(m_suppressVarSemi ? "\n" : ";\n");
    }
    शून्य visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    शून्य visit(AstDelay* nodep) override {
        माला_दो("");  // this is for proper alignment
        जर (nodep->isCycleDelay()) {
            माला_दो("##");
        } अन्यथा {
            माला_दो("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = असत्य;
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक fallDelayp = nodep->fallDelay()) {
            माला_दो("(");
            iterateConst(nodep->lhsp());
            माला_दो(", ");
            iterateConst(fallDelayp);
            माला_दो(")");
        } अन्यथा {
            iterateConst(nodep->lhsp());
        }
        जर (!m_suppressSemi) {
            माला_दो(";\n");
        } अन्यथा {
            माला_दो(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    शून्य visit(AstWait* nodep) override {
        माला_दो("wait(");
        iterateConst(nodep->condp());
        माला_दो(")");
        जर (nodep->stmtsp()) {
            माला_दो(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } अन्यथा {
            माला_दो(";\n");
        }
    }
    शून्य visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        माला_दो("");  // this is for proper alignment
        माला_दो("#");
        iterateConst(methodp->pinsp());
    }
    शून्य visit(AstParseRef* nodep) override { माला_दो(nodep->prettyName()); }
    शून्य visit(AstVarScope*) override {}
    शून्य visit(AstTraceDecl*) override {}
    शून्य visit(AstTraceInc*) override {}
    // NOPs
    शून्य visit(AstPragma*) override {}
    शून्य visit(AstStmtPragma*) override {}
    शून्य visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    शून्य visit(Ast_गाठ* nodep) override {
        माला_दो("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        जर (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

सार्वजनिक:
    explicit EmitVBaseVisitorConst(तार्किक suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = पूर्वनिर्धारित;
};

//######################################################################
// Emit to an output file

वर्ग EmitVFileVisitor final : सार्वजनिक EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    शून्य putsNoTracking(स्थिरांक string& str) override { m_of.putsNoTracking(str); }
    शून्य माला_दो(स्थिरांक string& str) override { m_of.माला_दो(str); }
    शून्य putbs(स्थिरांक string& str) override { m_of.putbs(str); }
    शून्य putfs(Ast_गाठ*, स्थिरांक string& str) override { putbs(str); }
    शून्य putqs(Ast_गाठ*, स्थिरांक string& str) override { putbs(str); }

सार्वजनिक:
    EmitVFileVisitor(Ast_गाठ* nodep, V3OutVFile& of, तार्किक suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

वर्ग EmitVStreamVisitor final : सार्वजनिक EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    स्थिरांक तार्किक m_tracking;  // Use line tracking
    // METHODS
    शून्य putsNoTracking(स्थिरांक string& str) override { m_os.putsNoTracking(str); }
    शून्य माला_दो(स्थिरांक string& str) override {
        m_tracking ? m_os.माला_दो(str) : m_os.putsNoTracking(str);
    }
    शून्य putbs(स्थिरांक string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    शून्य putfs(Ast_गाठ*, स्थिरांक string& str) override { putbs(str); }
    शून्य putqs(Ast_गाठ*, स्थिरांक string& str) override { putbs(str); }

सार्वजनिक:
    EmitVStreamVisitor(स्थिरांक Ast_गाठ* nodep, std::ostream& os, तार्किक tracking, तार्किक suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<Ast_गाठ*>(nodep));
    }
    ~EmitVStreamVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// EmitV class functions

शून्य V3EmitV::verilog_For_वृक्ष(स्थिरांक Ast_गाठ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ असत्य, असत्य}; }
}

शून्य V3EmitV::debug_Verilog_For_वृक्ष(स्थिरांक Ast_गाठ* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ सत्य, सत्य}; }
}

std::string V3EmitV::debug_Verilog_For_वृक्ष(स्थिरांक Ast_गाठ* nodep) {
    std::stringstream ss;
    debug_Verilog_For_वृक्ष(nodep, ss);
    परत ss.str();
}

शून्य V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    साठी (Ast_गाठ_File* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), गाठ_File)) {
        AstVFile* स्थिरांक vfilep = VN_CAST(filep, VFile);
        जर (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.माला_दो("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, असत्य};
        }
    }
}

शून्य V3EmitV::debugEmitV(स्थिरांक string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, सत्य};
}
