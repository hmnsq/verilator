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
    AstSenTree* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    منطقي m_suppressSemi = خطأ;  // Non-statement, don't print ;
    منطقي m_suppressVarSemi = خطأ;  // Suppress emitting semicolon for AstVars
    منطقي m_suppressSampled = خطأ;  // Suppress emitting sampled in assertion properties
    منطقي m_arrayPost = خطأ;  // Print array information that goes after identifier (vs after)
    منطقي m_prefixed = 𐡒𐡔𐡈;  // Whether constants need to be prefixed
    std::deque<AstNodeArrayDType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    virtual فراغ puts(ثابت string& str) = 0;
    virtual فراغ putbs(ثابت string& str) = 0;
    virtual فراغ putfs(AstNode* nodep, ثابت string& str) = 0;  // Fileline and node %% mark
    virtual فراغ putqs(AstNode* nodep, ثابت string& str) = 0;  // Fileline quiet w/o %% mark
    virtual فراغ putsNoTracking(ثابت string& str) = 0;
    virtual فراغ putsQuoted(ثابت string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    فراغ iterateAndCommaConstNull(AstNode* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            𐡄𐡍 (nodep->nextp()) puts(", ");
        }
    }
    فراغ emitPacked() {
        لكل (AstNodeArrayDType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    فراغ emitNodesWithText(AstNode* nodesp, منطقي tracking, ثابت std::string& separator) {
        لكل (AstNode* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            𐡄𐡍 (ثابت AstText* ثابت textp = VN_CAST(nodep, Text)) {
                𐡄𐡍 (tracking) {
                    puts(textp->text());
                } وإلا {
                    putsNoTracking(textp->text());
                }
            } وإلا {
                iterateConst(nodep);
            }
            𐡄𐡍 (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    فراغ visit(AstNodeModule* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    فراغ visit(AstPort* nodep) override {}
    فراغ visit(AstNodeFTask* nodep) override {
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
        𐡄𐡍 (nodep->condsp()) {
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
            m_suppressSemi = 𐡒𐡔𐡈;
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
        𐡄𐡍 (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ visit(AstBegin* nodep) override {
        𐡄𐡍 (nodep->name() == "") {
            putbs("begin\n");
        } وإلا {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    فراغ visit(AstFork* nodep) override {
        𐡄𐡍 (nodep->name() == "") {
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
        𐡄𐡍 (ثابت AstAssignW* ثابت ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            𐡄𐡍 (!ap->nextp()) {
                putfs(nodep, "assign ");
                𐡄𐡍 (AstNode* ثابت tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                𐡄𐡍 (!m_suppressSemi) puts(";\n");
                أرجع;
            }
        }
        putfs(nodep, "always ");
        𐡄𐡍 (m_sentreep) {
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
    فراغ visit(AstNodeAssign* nodep) override {
        𐡄𐡍 (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        𐡄𐡍 (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        لكل (AstNode* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        𐡄𐡍 (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        𐡄𐡍 (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        𐡄𐡍 (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstBreak*) override {
        putbs("break");
        𐡄𐡍 (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstSenTree* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        لكل (AstNode* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            𐡄𐡍 (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    فراغ visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        𐡄𐡍 (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        𐡄𐡍 (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    فراغ visit(AstCase* nodep) override {
        putfs(nodep, "");
        𐡄𐡍 (nodep->priorityPragma()) puts("priority ");
        𐡄𐡍 (nodep->uniquePragma()) puts("unique ");
        𐡄𐡍 (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        𐡄𐡍 (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            𐡄𐡍 (nodep->fullPragma()) puts(" full_case");
            𐡄𐡍 (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    فراغ visit(AstCaseItem* nodep) override {
        𐡄𐡍 (nodep->condsp()) {
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
        𐡄𐡍 (!m_suppressSemi) puts(";\n");
    }
    فراغ visit(AstNodeCoverDecl*) override {}  // N/A
    فراغ visit(AstCoverInc*) override {}  // N/A
    فراغ visit(AstCoverToggle*) override {}  // N/A

    فراغ visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        𐡄𐡍 (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    فراغ visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        𐡄𐡍 (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    فراغ emitCoverageOption(AstNode* nodep, منطقي typeOption, VCoverOptionType optType,
                            AstNodeExpr* valuep) {
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
        𐡄𐡍 (nodep->isArray()) {
            puts("[");
            𐡄𐡍 (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        𐡄𐡍 (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } وإلا 𐡄𐡍 (nodep->transp()) {
            puts(" = ");
            لكل (AstNode* setp = nodep->transp(); setp; setp = setp->nextp()) {
                𐡄𐡍 (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } وإلا 𐡄𐡍 (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } وإلا 𐡄𐡍 (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            لكل (AstNode* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                𐡄𐡍 (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        𐡄𐡍 (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    فراغ visit(AstCoverWith* nodep) override {
        ثابت منطقي rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        𐡄𐡍 (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        𐡄𐡍 (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    فراغ visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        𐡄𐡍 (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        𐡄𐡍 (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    فراغ visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        𐡄𐡍 (nodep->iffp()) {
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
        𐡄𐡍 (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        𐡄𐡍 (nodep->countp()) {
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
        لكل (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            𐡄𐡍 (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        𐡄𐡍 (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        𐡄𐡍 (nodep->binsp() || nodep->optionsp()) {
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
        لكل (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            𐡄𐡍 (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    فراغ visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    فراغ visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        𐡄𐡍 (AstConst* ثابت lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->num().toString());
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
    فراغ visit(AstValuePlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    فراغ visitNodeDisplay(AstNode* nodep, AstNode* fileOrStrgp, ثابت string& text,
                          AstNode* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        𐡄𐡍 (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        لكل (AstNode* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    فراغ visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    فراغ visit(AstDisplay* nodep) override {
        visitNodeDisplay(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    فراغ visit(AstElabDisplay* nodep) override {
        visitNodeDisplay(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    فراغ visit(AstFScanF* nodep) override {
        visitNodeDisplay(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    فراغ visit(AstSScanF* nodep) override {
        visitNodeDisplay(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    فراغ visit(AstSFormat* nodep) override {
        visitNodeDisplay(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    فراغ visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    فراغ visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    فراغ visit(AstSFormatF* nodep) override {
        visitNodeDisplay(nodep, nullptr, nodep->text(), nodep->exprsp());
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
        𐡄𐡍 (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    فراغ visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        𐡄𐡍 (nodep->filep()) iterateAndNextConstNull(nodep->filep());
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
    فراغ visit(AstNodeReadWriteMem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        𐡄𐡍 (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        𐡄𐡍 (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        𐡄𐡍 (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        𐡄𐡍 (nodep->msbp()) {
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
        𐡄𐡍 (AstLoopTest* ثابت testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            أرجع;
        }
        // Special case when the AstLoopTest is last for output readability
        𐡄𐡍 (AstNode* lastp = nodep->stmtsp()) {
            طالما (AstNode* ثابت nextp = lastp->nextp()) lastp = nextp;
            𐡄𐡍 (AstLoopTest* ثابت testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                لكل (AstNode* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                أرجع;
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
    فراغ visit(AstNodeIf* nodep) override {
        putfs(nodep, "");
        𐡄𐡍 (ثابت AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            𐡄𐡍 (ifp->priorityPragma()) puts("priority ");
            𐡄𐡍 (ifp->uniquePragma()) puts("unique ");
            𐡄𐡍 (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        𐡄𐡍 (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    فراغ visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        𐡄𐡍 (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            𐡄𐡍 (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    فراغ visit(AstSampled* nodep) override {
        𐡄𐡍 (!m_suppressSampled) {
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
        𐡄𐡍 (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    فراغ visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        𐡄𐡍 (nodep->sentreep()) {
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
        𐡄𐡍 (nodep->sentreep()) {
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
    فراغ visit(AstStmtExpr* nodep) override {
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
        emitNodesWithText(nodep->nodesp(), 𐡒𐡔𐡈, nodep->separator());
        puts(nodep->suffix());
    }
    فراغ visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), 𐡒𐡔𐡈, "");
        puts(");\n");
    }
    فراغ visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), 𐡒𐡔𐡈, "");
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
    فراغ visit(AstExprStmt* nodep) override {
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
    virtual فراغ emitVerilogFormat(AstNode* nodep, ثابت string& format, AstNode* lhsp = nullptr,
                                   AstNode* ثابت rhsp = nullptr, AstNode* thsp = nullptr,
                                   AstNode* fhsp = nullptr) {
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
            𐡄𐡍 (!inPct && c == '%') {
                inPct = 𐡒𐡔𐡈;
            } وإلا 𐡄𐡍 (!inPct) {  // Normal text
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

    فراغ visit(AstNodeTermop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    فراغ visit(AstNodeUniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    فراغ visit(AstNodeBiop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    فراغ visit(AstNodeTriop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    فراغ visit(AstNodeQuadop* nodep) override {
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
        𐡄𐡍 (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    فراغ visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        لكل (ثابت AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            𐡄𐡍 (itemp != nodep->itemsp()) putbs(", ");
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
        𐡄𐡍 (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->leftConst()));
            puts(":");
            puts(cvtToStr(nodep->rightConst()));
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
        AstNodeDType* ثابت dtypep = nodep->fromp()->dtypep();
        𐡄𐡍 (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* ثابت basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        𐡄𐡍 (VN_IS(nodep->lsbp(), Const)) {
            𐡄𐡍 (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } وإلا {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } وإلا {
            iterateAndNextConstNull(nodep->lsbp());
            𐡄𐡍 (offset != 0) {
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
    فراغ visit(AstNodeCoverOrAssert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        𐡄𐡍 (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } وإلا 𐡄𐡍 (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } وإلا 𐡄𐡍 (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        𐡄𐡍 (AstSampled* ثابت sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } وإلا {
            iterateAndNextConstNull(nodep->propp());
        }
        𐡄𐡍 (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } وإلا {
            puts(");\n");
        }

        𐡄𐡍 (ثابت AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } وإلا 𐡄𐡍 (ثابت AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    فراغ visit(AstAssocArrayDType* nodep) override {
        𐡄𐡍 (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[");
            iterateConst(nodep->keyDTypep());
            puts("]");
            m_arrayPost = 𐡒𐡔𐡈;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstBasicDType* nodep) override {
        𐡄𐡍 (m_arrayPost) أرجع;
        putfs(nodep, nodep->prettyName());
        𐡄𐡍 (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        𐡄𐡍 (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) أرجع;
        emitPacked();
        𐡄𐡍 (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } وإلا 𐡄𐡍 (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->left()));
            puts(":");
            puts(cvtToStr(nodep->right()));
            puts("] ");
        }
    }
    فراغ visit(AstConstDType* nodep) override {
        𐡄𐡍 (m_arrayPost) أرجع;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    فراغ visit(AstDynArrayDType* nodep) override {
        𐡄𐡍 (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstEnumDType* nodep) override {
        𐡄𐡍 (m_arrayPost) أرجع;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        𐡄𐡍 (AstNodeModule* ثابت classOrPackagep = nodep->classOrPackagep()) {
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
        𐡄𐡍 (nodep->nextp()) puts(",");
        puts("\n");
    }
    فراغ visit(AstNodeArrayDType* nodep) override {
        𐡄𐡍 (!m_arrayPost) {
            𐡄𐡍 (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.push_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } وإلا {
            𐡄𐡍 (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = خطأ;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = 𐡒𐡔𐡈;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstIfaceRefDType* nodep) override {
        𐡄𐡍 (m_arrayPost) {
            puts(" (");
            𐡄𐡍 (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } وإلا {
                puts("????");
            }
            puts(")");
            أرجع;
        }
        puts(nodep->ifaceName());
    }
    فراغ visit(AstRefDType* nodep) override {
        𐡄𐡍 (nodep->subDTypep()) {
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
        𐡄𐡍 (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } وإلا {
            puts("????");
        }
        puts(");\n");
    }
    فراغ visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        𐡄𐡍 (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = 𐡒𐡔𐡈;
            iterateConst(nodep->varp());
        } وإلا {
            puts(nodep->prettyName());
        }
        𐡄𐡍 (nodep->nextp()) puts(", ");
    }
    فراغ visit(AstNodeUOrStructDType* nodep) override {
        𐡄𐡍 (m_arrayPost) أرجع;
        puts(nodep->verilogKwd() + " ");
        𐡄𐡍 (nodep->packed()) puts("packed ");
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
        𐡄𐡍 (m_arrayPost) أرجع;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    فراغ visit(AstQueueDType* nodep) override {
        𐡄𐡍 (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } وإلا {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = خطأ;
            puts("[$");
            𐡄𐡍 (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = 𐡒𐡔𐡈;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        𐡄𐡍 (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } وإلا {
            putfs(nodep, nodep->prettyName());
        }
        𐡄𐡍 (!VN_IS(nodep->taskp(), Property)) {
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
        𐡄𐡍 (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            لكل (AstNode* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                𐡄𐡍 (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        𐡄𐡍 (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        𐡄𐡍 (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        𐡄𐡍 (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
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
        𐡄𐡍 (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
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
            m_suppressSemi = 𐡒𐡔𐡈;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    فراغ visit(AstVarRef* nodep) override {
        𐡄𐡍 (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } وإلا {
            𐡄𐡍 (nodep->varp()) {
                𐡄𐡍 (nodep->selfPointer().isEmpty()) {
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
        𐡄𐡍 (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } وإلا {
            puts(nodep->prettyName());
        }
    }
    فراغ visit(AstConst* nodep) override { putfs(nodep, nodep->num().ascii(m_prefixed, 𐡒𐡔𐡈)); }
    فراغ visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    فراغ visit(AstTopScope* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    فراغ visit(AstVar* nodep) override {
        𐡄𐡍 (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = خطأ;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = 𐡒𐡔𐡈;
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
        𐡄𐡍 (nodep->isCycleDelay()) {
            puts("##");
        } وإلا {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = خطأ;
        𐡄𐡍 (AstNodeExpr* ثابت fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } وإلا {
            iterateConst(nodep->lhsp());
        }
        𐡄𐡍 (!m_suppressSemi) {
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
        𐡄𐡍 (nodep->stmtsp()) {
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
    فراغ visit(AstNode* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        𐡄𐡍 (!m_suppressUnknown) {
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
    فراغ putfs(AstNode*, ثابت string& str) override { putbs(str); }
    فراغ putqs(AstNode*, ثابت string& str) override { putbs(str); }

عام:
    EmitVFileVisitor(AstNode* nodep, V3OutVFile& of, منطقي suppressUnknown)
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
    فراغ putfs(AstNode*, ثابت string& str) override { putbs(str); }
    فراغ putqs(AstNode*, ثابت string& str) override { putbs(str); }

عام:
    EmitVStreamVisitor(ثابت AstNode* nodep, std::ostream& os, منطقي tracking, منطقي suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<AstNode*>(nodep));
    }
    ~EmitVStreamVisitor() override = افتراضي;
};

//######################################################################
// EmitV class functions

فراغ V3EmitV::verilogForTree(ثابت AstNode* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ خطأ, خطأ}; }
}

فراغ V3EmitV::debugVerilogForTree(ثابت AstNode* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ 𐡒𐡔𐡈, 𐡒𐡔𐡈}; }
}

std::string V3EmitV::debugVerilogForTree(ثابت AstNode* nodep) {
    std::stringstream ss;
    debugVerilogForTree(nodep, ss);
    أرجع ss.str();
}

فراغ V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    لكل (AstNodeFile* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), NodeFile)) {
        AstVFile* ثابت vfilep = VN_CAST(filep, VFile);
        𐡄𐡍 (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, خطأ};
        }
    }
}

فراغ V3EmitV::debugEmitV(ثابت string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, 𐡒𐡔𐡈};
}
