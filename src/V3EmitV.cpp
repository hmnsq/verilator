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

#include "V3PchAstMT.h"

#include "V3EmitV.h"

#include "V3EmitCBase.h"

#include <unordered_map>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

class EmitVBaseVisitorConst VL_NOT_FINAL : public VNVisitorConst {
    // STATE - across all visitors
    const bool m_suppressUnknown;  // Do not error on unknown node

    // STATE - for current visit position (use VL_RESTORER)
    AstSenTree* m_sentreep = nullptr;  // Domain for printing one a ALWAYS under a ACTIVE
    bool m_suppressSemi = false;  // Non-statement, don't print ;
    bool m_suppressVarSemi = false;  // Suppress emitting semicolon for AstVars
    bool m_suppressSampled = false;  // Suppress emitting sampled in assertion properties
    bool m_arrayPost = false;  // Print array information that goes after identifier (vs after)
    bool m_prefixed = true;  // Whether constants need to be prefixed
    std::deque<AstNodeArrayDType*> m_packedps;  // Packed arrays to print with BasicDType
    std::unordered_map<AstJumpBlock*, size_t> m_labelNumbers;  // Label numbers for JumpBlocks

    // METHODS
    virtual void puts(const string& str) = 0;
    virtual void putbs(const string& str) = 0;
    virtual void putfs(AstNode* nodep, const string& str) = 0;  // Fileline and node %% mark
    virtual void putqs(AstNode* nodep, const string& str) = 0;  // Fileline quiet w/o %% mark
    virtual void putsNoTracking(const string& str) = 0;
    virtual void putsQuoted(const string& str) {
        // Quote \ and " for use inside C programs
        // Don't use to quote a filename for #include - #include doesn't \ escape.
        // Duplicate in V3File - here so we can print to string
        putsNoTracking("\"");
        putsNoTracking(V3OutFormatter::quoteNameControls(str));
        putsNoTracking("\"");
    }

    void iterateAndCommaConstNull(AstNode* nodep) {
        සඳහා (; nodep; nodep = nodep->nextp()) {
            iterateConst(nodep);
            නම් (nodep->nextp()) puts(", ");
        }
    }
    void emitPacked() {
        සඳහා (AstNodeArrayDType* packedp : m_packedps) {
            puts(" ");
            iterateConstNull(packedp->rangep());
        }
        m_packedps.clear();
    }
    void emitNodesWithText(AstNode* nodesp, bool tracking, const std::string& separator) {
        සඳහා (AstNode* nodep = nodesp; nodep; nodep = nodep->nextp()) {
            නම් (const AstText* const textp = VN_CAST(nodep, Text)) {
                නම් (tracking) {
                    puts(textp->text());
                } නැතහොත් {
                    putsNoTracking(textp->text());
                }
            } නැතහොත් {
                iterateConst(nodep);
            }
            නම් (nodep->nextp()) puts(separator);
        }
    }

    // VISITORS
    void visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    void visit(AstNodeModule* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + EmitCUtil::prefixNameProtect(nodep) + ";\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }
    void visit(AstPort* nodep) override {}
    void visit(AstNodeFTask* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
        // Only putfs the first time for each visitor; later for same node is putqs
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end" + nodep->verilogKwd() + "\n");
    }

    void visit(AstGenBlock* nodep) override {
        const std::string name = nodep->name().empty() ? "" : " : " + nodep->name();
        putbs("/* generate */ begin" + name + '\n');
        iterateChildrenConst(nodep);
        puts("end" + name + '\n');
    }
    void visit(AstGenCase* nodep) override {
        putfs(nodep, "/* generate */ case (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    void visit(AstGenCaseItem* nodep) override {
        නම් (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } නැතහොත් {
            putbs("default");
        }
        iterateAndNextConstNull(nodep->itemsp());
    }
    void visit(AstGenFor* nodep) override {
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
    void visit(AstGenIf* nodep) override {
        putfs(nodep, "");
        puts("/* generate */ if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        නම් (nodep->elsesp()) {
            putqs(nodep, "end else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    void visit(AstBegin* nodep) override {
        නම් (nodep->name() == "") {
            putbs("begin\n");
        } නැතහොත් {
            putbs("begin : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts("end\n");
    }
    void visit(AstFork* nodep) override {
        නම් (nodep->name() == "") {
            putbs("fork\n");
        } නැතහොත් {
            putbs("fork : " + nodep->name() + "\n");
        }
        iterateChildrenConst(nodep);
        puts(nodep->joinType().verilogKwd());
        puts("\n");
    }
    void visit(AstFinal* nodep) override {
        putfs(nodep, "final begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    void visit(AstInitial* nodep) override {
        putfs(nodep, "initial begin\n");
        iterateChildrenConst(nodep);
        putqs(nodep, "end\n");
    }
    void visit(AstInitialAutomatic* nodep) override { iterateChildrenConst(nodep); }
    void visit(AstInitialStatic* nodep) override { iterateChildrenConst(nodep); }
    void visit(AstInitialAutomaticStmt* nodep) override { iterateChildrenConst(nodep); }
    void visit(AstInitialStaticStmt* nodep) override { iterateChildrenConst(nodep); }
    void visit(AstAlways* nodep) override {
        නම් (const AstAssignW* const ap = VN_CAST(nodep->stmtsp(), AssignW)) {
            නම් (!ap->nextp()) {
                putfs(nodep, "assign ");
                නම් (AstNode* const tcp = ap->timingControlp()) {
                    iterateAndNextConstNull(tcp);
                    putbs(" ");
                }
                iterateAndNextConstNull(ap->lhsp());
                putbs(" = ");
                iterateAndNextConstNull(ap->rhsp());
                නම් (!m_suppressSemi) puts(";\n");
                ආපසු;
            }
        }
        putfs(nodep, "always ");
        නම් (m_sentreep) {
            iterateAndNextConstNull(m_sentreep);
        }  // In active
        නැතහොත් {
            iterateAndNextConstNull(nodep->sentreep());
        }
        putbs(" begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    void visit(AstAlwaysPre* nodep) override {
        putfs(nodep, "always /* PRE */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    void visit(AstAlwaysPost* nodep) override {
        putfs(nodep, "always /* POST */ begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    void visit(AstNodeAssign* nodep) override {
        නම් (VN_IS(nodep, AssignForce)) puts("force ");
        iterateAndNextConstNull(nodep->lhsp());
        putfs(nodep, " " + nodep->verilogKwd() + " ");
        iterateAndNextConstNull(nodep->rhsp());
        නම් (!m_suppressSemi) puts(";\n");
    }
    void visit(AstAlias* nodep) override {
        putbs("alias ");
        iterateConst(nodep->itemsp());
        සඳහා (AstNode* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            putfs(nodep, " = ");
            iterateConst(itemp);
        }
        නම් (!m_suppressSemi) puts(";\n");
    }
    void visit(AstAssignW* nodep) override {
        putfs(nodep, "continuous assign ");
        iterateAndNextConstNull(nodep->lhsp());
        putbs(" = ");
        iterateAndNextConstNull(nodep->rhsp());
        නම් (!m_suppressSemi) puts(";\n");
    }
    void visit(AstRelease* nodep) override {
        puts("release ");
        iterateAndNextConstNull(nodep->lhsp());
        නම් (!m_suppressSemi) puts(";\n");
    }
    void visit(AstBreak*) override {
        putbs("break");
        නම් (!m_suppressSemi) puts(";\n");
    }
    void visit(AstSenTree* nodep) override {
        // AstSenItem is called for dumping in isolation by V3Order
        putfs(nodep, "@(");
        සඳහා (AstNode* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
            iterateConst(expp);
            නම් (expp->nextp()) putqs(expp->nextp(), " or ");
        }
        puts(")");
    }
    void visit(AstSenItem* nodep) override {
        putfs(nodep, "");
        නම් (nodep->edgeType() != VEdgeType::ET_CHANGED) puts(nodep->edgeType().verilogKwd());
        නම් (nodep->sensp()) puts(" ");
        iterateChildrenConst(nodep);
    }
    void visit(AstCReset* /*nodep*/) override { puts("/*CRESET*/"); }
    void visit(AstCase* nodep) override {
        putfs(nodep, "");
        නම් (nodep->priorityPragma()) puts("priority ");
        නම් (nodep->uniquePragma()) puts("unique ");
        නම් (nodep->unique0Pragma()) puts("unique0 ");
        puts(nodep->verilogKwd());
        puts(" (");
        iterateAndNextConstNull(nodep->exprp());
        puts(")\n");
        නම් (nodep->fullPragma() || nodep->parallelPragma()) {
            puts(" // synopsys");
            නම් (nodep->fullPragma()) puts(" full_case");
            නම් (nodep->parallelPragma()) puts(" parallel_case");
        }
        iterateAndNextConstNull(nodep->itemsp());
        putqs(nodep, "endcase\n");
    }
    void visit(AstCaseItem* nodep) override {
        නම් (nodep->condsp()) {
            iterateAndNextConstNull(nodep->condsp());
        } නැතහොත් {
            putbs("default");
        }
        putfs(nodep, ": begin ");
        iterateAndNextConstNull(nodep->stmtsp());
        putqs(nodep, "end\n");
    }
    void visit(AstComment* nodep) override {
        puts("// "s + nodep->name() + "\n");
        iterateChildrenConst(nodep);
    }
    void visit(AstContinue*) override {
        putbs("continue");
        නම් (!m_suppressSemi) puts(";\n");
    }
    void visit(AstNodeCoverDecl*) override {}  // N/A
    void visit(AstCoverInc*) override {}  // N/A
    void visit(AstCoverToggle*) override {}  // N/A

    void visit(AstCovergroup* nodep) override {
        // AstCovergroup appears as a member inside the lowered AstClass body.
        // The outer covergroup/endcovergroup wrapper is already emitted by the
        // AstNodeModule visitor (verilogKwd()="covergroup" on AstClass::isCovergroup).
        // Here we only emit the clocking event, if any.
        නම් (nodep->eventp()) {
            putfs(nodep, "");
            iterateConst(nodep->eventp());
        }
    }
    void visit(AstCoverpoint* nodep) override {
        putfs(nodep, nodep->name() + ": coverpoint ");
        iterateAndNextConstNull(nodep->exprp());
        නම් (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}");
        }
        puts(";\n");
    }
    // An option or type_option assignment, from a covergroup, coverpoint, or cross
    void emitCoverageOption(AstNode* nodep, bool typeOption, VCoverOptionType optType,
                            AstNodeExpr* valuep) {
        putfs(nodep,
              std::string{typeOption ? "type_option." : "option."} + optType.ascii() + " = ");
        iterateConst(valuep);
        puts(";\n");
    }
    void visit(AstCgOptionAssign* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    void visit(AstCoverOption* nodep) override {
        emitCoverageOption(nodep, nodep->typeOption(), nodep->optType(), nodep->valuep());
    }
    void visit(AstCoverBin* nodep) override {
        putfs(nodep, std::string{nodep->isWildcard() ? "wildcard " : ""}
                         + nodep->binsType().verilogKwd() + " ");
        puts(nodep->name());
        නම් (nodep->isArray()) {
            puts("[");
            නම් (nodep->arraySizep()) iterateConst(nodep->arraySizep());
            puts("]");
        }
        නම් (nodep->binsType() == VCoverBinsType::BINS_DEFAULT) {
            puts(" = default");
        } නැතහොත් නම් (nodep->transp()) {
            puts(" = ");
            සඳහා (AstNode* setp = nodep->transp(); setp; setp = setp->nextp()) {
                නම් (setp != nodep->transp()) puts(", ");
                iterateConst(setp);
            }
        } නැතහොත් නම් (VN_IS(nodep->rangesp(), CoverWith)) {
            puts(" = ");
            iterateConst(nodep->rangesp());
        } නැතහොත් නම් (nodep->rangesp()) {  // LCOV_EXCL_BR_LINE - false: CoverBin always has
                                        // transp/rangesp/default
            puts(" = {");
            සඳහා (AstNode* rangep = nodep->rangesp(); rangep; rangep = rangep->nextp()) {
                නම් (rangep != nodep->rangesp()) puts(", ");
                iterateConst(rangep);
            }
            puts("}");
        }
        නම් (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    void visit(AstCoverWith* nodep) override {
        const bool rangeList = !VN_IS(nodep->subp(), CoverpointRef);
        නම් (rangeList) putfs(nodep, "{");
        iterateAndCommaConstNull(nodep->subp());
        නම් (rangeList) puts("}");
        puts(" with (");
        iterateConst(nodep->filterp());
        puts(")");
    }
    void visit(AstCoverBinsof* nodep) override {
        putfs(nodep, nodep->isNegated() ? "!binsof(" : "binsof(");
        iterateConst(nodep->pointp());
        නම් (!nodep->name().empty()) puts("." + nodep->name());
        puts(")");
        නම් (nodep->rangesp()) {
            puts(" intersect {");
            iterateAndCommaConstNull(nodep->rangesp());
            puts("}");
        }
    }
    void visit(AstCoverCrossBin* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " " + nodep->name() + " = ");
        iterateConstNull(nodep->selectp());
        නම් (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        puts(";\n");
    }
    void visit(AstCoverCrossRef* nodep) override { putfs(nodep, nodep->name()); }
    void visit(AstCoverCrossSelect* nodep) override {
        putfs(nodep, "(");
        iterateConstNull(nodep->lhsp());
        putbs(" " + nodep->verilogKwd() + " ");
        iterateConstNull(nodep->rhsp());
        puts(")");
    }
    void visit(AstCoverCrossWith* nodep) override {
        // A selection with a count is filtered further only in parentheses
        නම් (nodep->countp()) putfs(nodep, "(");
        iterateConstNull(nodep->selectp());
        putfs(nodep, " with (");
        iterateConst(nodep->filterp());
        puts(")");
        නම් (nodep->countp()) {
            puts(" matches ");
            iterateConst(nodep->countp());
            puts(")");
        }
    }
    void visit(AstCoverpointRef* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->exprp());
    }
    void visit(AstCoverCross* nodep) override {
        putfs(nodep, nodep->name() + ": cross ");
        සඳහා (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            නම් (itemp != nodep->itemsp()) puts(", ");
            iterateConst(itemp);
        }
        නම් (nodep->iffp()) {
            puts(" iff (");
            iterateConst(nodep->iffp());
            puts(")");
        }
        නම් (nodep->binsp() || nodep->optionsp()) {
            puts(" {\n");
            iterateAndNextConstNull(nodep->optionsp());
            iterateAndNextConstNull(nodep->binsp());
            puts("}\n");
        } නැතහොත් {
            puts(";\n");
        }
    }
    void visit(AstCoverTransSet* nodep) override {
        puts("(");
        සඳහා (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            නම් (itemp != nodep->itemsp()) puts(" => ");
            iterateConst(itemp);
        }
        puts(")");
    }
    void visit(AstCoverTransItem* nodep) override { iterateChildrenConst(nodep); }

    void visit(AstCvtPackString* nodep) override {
        putfs(nodep, "");
        නම් (AstConst* const lhsConstp = VN_CAST(nodep->lhsp(), Const)) {
            putsQuoted(lhsConstp->num().toString());
        } නැතහොත් {
            puts("string'(");
            iterateAndNextConstNull(nodep->lhsp());
            puts(")");
        }
    }
    void visit(AstTestPlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateChildrenConst(nodep);
        puts(")");
    }
    void visit(AstValuePlusArgs* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateConstNull(nodep->searchp());
        putbs(", ");
        iterateConstNull(nodep->outp());
        puts(")");
    }
    void visitNodeDisplay(AstNode* nodep, AstNode* fileOrStrgp, const string& text,
                          AstNode* exprsp) {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        නම් (fileOrStrgp) {
            iterateConstNull(fileOrStrgp);
            putbs(", ");
        }
        putsQuoted(text);
        සඳහා (AstNode* expp = exprsp; expp; expp = expp->nextp()) {
            puts(", ");
            iterateConstNull(expp);
        }
        puts(");\n");
    }
    void visit(AstDisable* nodep) override { putbs("disable " + nodep->name() + ";\n"); }
    void visit(AstDisplay* nodep) override {
        visitNodeDisplay(nodep, nodep->filep(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    void visit(AstElabDisplay* nodep) override {
        visitNodeDisplay(nodep, nullptr, nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    void visit(AstFScanF* nodep) override {
        visitNodeDisplay(nodep, nodep->filep(), nodep->text(), nodep->exprsp());
    }
    void visit(AstSScanF* nodep) override {
        visitNodeDisplay(nodep, nodep->fromp(), nodep->text(), nodep->exprsp());
    }
    void visit(AstSFormat* nodep) override {
        visitNodeDisplay(nodep, nodep->lhsp(), nodep->fmtp()->text(), nodep->fmtp()->exprsp());
    }
    void visit(AstToStringN* nodep) override { iterateConst(nodep->lhsp()); }
    void visit(AstSFormatArg* nodep) override { iterateConst(nodep->exprp()); }
    void visit(AstSFormatF* nodep) override {
        visitNodeDisplay(nodep, nullptr, nodep->text(), nodep->exprsp());
    }
    void visit(AstFOpen* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        iterateAndNextConstNull(nodep->modep());
        puts(");\n");
    }
    void visit(AstFOpenMcd* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->filenamep());
        puts(");\n");
    }
    void visit(AstFClose* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        නම් (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    void visit(AstFFlush* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        නම් (nodep->filep()) iterateAndNextConstNull(nodep->filep());
        puts(");\n");
    }
    void visit(AstJumpBlock* nodep) override {
        // Allocate label number
        const size_t n = m_labelNumbers.size();
        const bool newEntry = m_labelNumbers.emplace(nodep, n).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twide");
        // Emit
        putbs("begin : label" + std::to_string(n) + "\n");
        iterateAndNextConstNull(nodep->stmtsp());
        puts("end\n");
    }
    void visit(AstJumpGo* nodep) override {
        // Retrieve target label number - Sometimes EmitV is used by debug code,
        // so allow printing with an unknown target
        const auto it = m_labelNumbers.find(nodep->blockp());
        const std::string label
            = it != m_labelNumbers.end() ? "label" + std::to_string(it->second) : "<UNKNOWN>";
        putbs("disable " + label + ";\n");
    }
    void visit(AstNodeReadWriteMem* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        නම් (nodep->filenamep()) iterateAndNextConstNull(nodep->filenamep());
        putbs(", ");
        නම් (nodep->memp()) iterateAndNextConstNull(nodep->memp());
        නම් (nodep->lsbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->lsbp());
        }
        නම් (nodep->msbp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->msbp());
        }
        puts(");\n");
    }
    void visit(AstSysIgnore* nodep) override {
        putfs(nodep, nodep->verilogKwd());
        putbs("(");
        iterateAndNextConstNull(nodep->exprsp());
        puts(");\n");
    }
    void visit(AstRepeat* nodep) override {
        putfs(nodep, "repeat (");
        iterateAndNextConstNull(nodep->countp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putfs(nodep, "end\n");
    }
    void visit(AstLoop* nodep) override {
        // Special case when the AstLoopTest is first for output readability
        නම් (AstLoopTest* const testp = VN_CAST(nodep->stmtsp(), LoopTest)) {
            putfs(nodep, "while (");
            iterateConst(testp->condp());
            puts(") begin\n");
            iterateAndNextConstNull(testp->nextp());
            puts("end\n");
            ආපසු;
        }
        // Special case when the AstLoopTest is last for output readability
        නම් (AstNode* lastp = nodep->stmtsp()) {
            අතරතුර (AstNode* const nextp = lastp->nextp()) lastp = nextp;
            නම් (AstLoopTest* const testp = VN_CAST(lastp, LoopTest)) {
                putfs(nodep, "do begin\n");
                සඳහා (AstNode* p = nodep->stmtsp(); p != lastp; p = p->nextp()) iterateConst(p);
                puts("end while (");
                iterateConst(testp->condp());
                puts(")\n");
                ආපසු;
            }
        }
        // Generic case
        putfs(nodep, "while (true) begin\n");
        iterateAndNextConstNull(nodep->stmtsp());
        iterateAndNextConstNull(nodep->contsp());
        putfs(nodep, "end\n");
    }
    void visit(AstLoopTest* nodep) override {
        putfs(nodep, "if (!(");
        iterateAndNextConstNull(nodep->condp());
        puts(")) break;\n");
    }
    void visit(AstNodeIf* nodep) override {
        putfs(nodep, "");
        නම් (const AstIf* const ifp = VN_CAST(nodep, If)) {
            නම් (ifp->priorityPragma()) puts("priority ");
            නම් (ifp->uniquePragma()) puts("unique ");
            නම් (ifp->unique0Pragma()) puts("unique0 ");
        }
        puts("if (");
        iterateAndNextConstNull(nodep->condp());
        puts(") begin\n");
        iterateAndNextConstNull(nodep->thensp());
        නම් (nodep->elsesp()) {
            putqs(nodep, "end\n");
            putqs(nodep, "else begin\n");
            iterateAndNextConstNull(nodep->elsesp());
        }
        putqs(nodep, "end\n");
    }
    void visit(AstPast* nodep) override {
        putfs(nodep, "$past(");
        iterateAndNextConstNull(nodep->exprp());
        නම් (nodep->ticksp() || nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->ticksp());
            නම් (nodep->sentreep()) {
                puts(", ");
                iterateAndNextConstNull(nodep->sentreep());
            }
        }
        puts(")");
    }
    void visit(AstSampled* nodep) override {
        නම් (!m_suppressSampled) {
            putfs(nodep, "$sampled(");
            iterateAndNextConstNull(nodep->exprp());
            puts(")");
        }
    }
    void visit(AstRising* nodep) override {
        putfs(nodep, "$rising(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    void visit(AstRose* nodep) override {
        putfs(nodep, "$rose(");
        iterateAndNextConstNull(nodep->exprp());
        නම් (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    void visit(AstFell* nodep) override {
        putfs(nodep, "$fell(");
        iterateAndNextConstNull(nodep->exprp());
        නම් (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    void visit(AstFalling* nodep) override {
        putfs(nodep, "$falling(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    void visit(AstFuture* nodep) override {
        putfs(nodep, "$future(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    void visit(AstStable* nodep) override {
        putfs(nodep, "$stable(");
        iterateAndNextConstNull(nodep->exprp());
        නම් (nodep->sentreep()) {
            puts(", ");
            iterateAndNextConstNull(nodep->sentreep());
        }
        puts(")");
    }
    void visit(AstSteady* nodep) override {
        putfs(nodep, "$steady(");
        iterateAndNextConstNull(nodep->exprp());
        puts(")");
    }
    void visit(AstReturn* nodep) override {
        putfs(nodep, "return ");
        iterateAndNextConstNull(nodep->lhsp());
        puts(";\n");
    }
    void visit(AstStop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog());
        puts(";\n");
    }
    void visit(AstFinish* nodep) override { putfs(nodep, "$finish;\n"); }
    void visit(AstFinishFork* nodep) override { putfs(nodep, "$finish;\n"); }
    void visit(AstStmtExpr* nodep) override {
        iterateConst(nodep->exprp());
        puts(";\n");
    }

    // Nodes involing AstText
    void visit(AstText* nodep) override {
        // All Text should be under TextBlock/CStmt/CStmtUser/CExpr/CExprUser
        nodep->v3fatalSrc("Text node in unexpected position");
    }
    void visit(AstTextBlock* nodep) override {
        VL_RESTORER(m_suppressVarSemi);
        m_suppressVarSemi = !nodep->separator().empty();
        puts(nodep->prefix());
        emitNodesWithText(nodep->nodesp(), true, nodep->separator());
        puts(nodep->suffix());
    }
    void visit(AstCStmt* nodep) override {
        putfs(nodep, "$_CSTMT(");
        emitNodesWithText(nodep->nodesp(), true, "");
        puts(");\n");
    }
    void visit(AstCExpr* nodep) override {
        putfs(nodep, "$_CEXPR(");
        emitNodesWithText(nodep->nodesp(), true, "");
        puts(")");
    }
    void visit(AstCStmtUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), false, "");
        puts(");\n");
    }
    void visit(AstCExprUser* nodep) override {
        putfs(nodep, "$c(");
        emitNodesWithText(nodep->nodesp(), false, "");
        puts(")");
    }

    void visit(AstScopeName* nodep) override {}
    void visit(AstExprStmt* nodep) override {
        putfs(nodep, "$_EXPRSTMT(\n");
        iterateAndNextConstNull(nodep->stmtsp());
        putbs(", ");
        iterateAndNextConstNull(nodep->resultp());
        puts(");\n");
    }

    void visit(AstCMethodHard* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->pinsp());
        puts(")");
        iterateConstNull(nodep->withp());
    }
    void visit(AstCMethodCall* nodep) override {
        iterateConst(nodep->fromp());
        puts("." + nodep->name() + "(");
        iterateAndCommaConstNull(nodep->argsp());
        puts(")");
    }

    // Operators
    virtual void emitVerilogFormat(AstNode* nodep, const string& format, AstNode* lhsp = nullptr,
                                   AstNode* const rhsp = nullptr, AstNode* thsp = nullptr,
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
        bool inPct = false;
        putbs("");
        සඳහා (const var_na c : format) {
            නම් (!inPct && c == '%') {
                inPct = true;
            } නැතහොත් නම් (!inPct) {  // Normal text
                string s;
                s += c;
                puts(s);
            } නැතහොත් {  // Format character
                inPct = false;
                switch (c) {
                pak_sa '%': puts("%"); bha_nga;
                pak_sa 'f': putfs(nodep, ""); bha_nga;
                pak_sa 'k': putbs(""); bha_nga;
                pak_sa 'l': {
                    UASSERT_OBJ(lhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(lhsp);
                    bha_nga;
                }
                pak_sa 'r': {
                    UASSERT_OBJ(rhsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(rhsp);
                    bha_nga;
                }
                pak_sa 't': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(thsp);
                    bha_nga;
                }
                pak_sa 'o': {
                    UASSERT_OBJ(thsp, nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(fhsp);
                    bha_nga;
                }
                pak_sa 'd': {
                    UASSERT_OBJ(nodep->dtypep(), nodep, "emitVerilog() references undef node");
                    iterateAndNextConstNull(nodep->dtypep());
                    bha_nga;
                }
                default: nodep->v3fatalSrc("Unknown emitVerilog format code: %" << c); bha_nga;
                }
            }
        }
    }

    void visit(AstNodeTermop* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }
    void visit(AstNodeUniop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp());
    }
    void visit(AstNodeBiop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp());
    }
    void visit(AstNodeTriop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(),
                          nodep->thsp());
    }
    void visit(AstNodeQuadop* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->lhsp(), nodep->rhsp(), nodep->thsp(),
                          nodep->fhsp());
    }
    void visit(AstMemberSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    void visit(AstStructSel* nodep) override {
        iterateConst(nodep->fromp());
        puts(".");
        puts(nodep->prettyName());
    }
    void visit(AstAttrOf* nodep) override {
        putfs(nodep, "$_ATTROF(");
        iterateAndNextConstNull(nodep->fromp());
        නම් (nodep->dimp()) {
            putbs(", ");
            iterateAndNextConstNull(nodep->dimp());
        }
        puts(")");
    }
    void visit(AstConsArray* nodep) override {
        putfs(nodep, "'{");
        සඳහා (const AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            නම් (itemp != nodep->itemsp()) putbs(", ");
            puts(cvtToStr(itemp->index()));
            puts(":");
            iterateConst(itemp->valuep());
        }
        puts("}");
    }
    void visit(AstCond* nodep) override {
        putbs("(");
        iterateAndNextConstNull(nodep->condp());
        putfs(nodep, " ? ");
        iterateAndNextConstNull(nodep->thenp());
        putbs(" : ");
        iterateAndNextConstNull(nodep->elsep());
        puts(")");
    }
    void visit(AstInsideRange* nodep) override {
        puts("[");
        iterateAndNextConstNull(nodep->lhsp());
        puts(":");
        iterateAndNextConstNull(nodep->rhsp());
        puts("]");
    }
    void visit(AstRange* nodep) override {
        puts("[");
        නම් (VN_IS(nodep->leftp(), Const) && VN_IS(nodep->rightp(), Const)) {
            // Looks nicer if we print [1:0] rather than [32'sh1:32sh0]
            puts(cvtToStr(nodep->leftConst()));
            puts(":");
            puts(cvtToStr(nodep->rightConst()));
        } නැතහොත් {
            iterateAndNextConstNull(nodep->leftp());
            puts(":");
            iterateAndNextConstNull(nodep->rightp());
        }
        puts("]");
    }
    void visit(AstRand* nodep) override {
        emitVerilogFormat(nodep, nodep->emitVerilog(), nodep->seedp());
    }
    void visit(AstSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        පූර්ණ_සංඛ්‍යා offset = 0;
        AstNodeDType* const dtypep = nodep->fromp()->dtypep();
        නම් (VN_IS(dtypep, BasicDType)) {
            AstBasicDType* const basicDtypep = VN_AS(dtypep, BasicDType);
            offset = basicDtypep->lo();
        }
        puts("[");
        නම් (VN_IS(nodep->lsbp(), Const)) {
            නම් (nodep->widthConst() == 1) {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            } නැතහොත් {
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + nodep->widthConst() + offset
                              - 1));
                puts(":");
                puts(cvtToStr(VN_AS(nodep->lsbp(), Const)->toSInt() + offset));
            }
        } නැතහොත් {
            iterateAndNextConstNull(nodep->lsbp());
            නම් (offset != 0) {
                puts(" + ");
                puts(cvtToStr(offset));
            }
            putfs(nodep, "+:");
            puts(cvtToStr(nodep->widthConst()));
        }
        puts("]");
    }
    void visit(AstSliceSel* nodep) override {
        iterateAndNextConstNull(nodep->fromp());
        puts(cvtToStr(nodep->declRange()));
    }
    void visit(AstThisRef* /*nodep*/) override { puts("this"); }
    void visit(AstTypedef* nodep) override {
        putfs(nodep, "typedef ");
        iterateConstNull(nodep->subDTypep());
        puts(" ");
        puts(nodep->prettyName());
        puts(";\n");
    }
    void visit(AstNodeCoverOrAssert* nodep) override {
        putfs(nodep, nodep->verilogKwd() + " ");
        නම් (nodep->userType() == VAssertType::OBSERVED_DEFERRED_IMMEDIATE) {
            puts("#0 ");
        } නැතහොත් නම් (nodep->userType() == VAssertType::FINAL_DEFERRED_IMMEDIATE) {
            puts("final ");
        } නැතහොත් නම් (nodep->userType() == VAssertType::CONCURRENT) {
            puts("property ");
        }
        iterateConstNull(nodep->sentreep());
        puts("(");
        නම් (AstSampled* const sampledp = VN_CAST(nodep->propp(), Sampled)) {
            iterateAndNextConstNull(sampledp->exprp());
        } නැතහොත් {
            iterateAndNextConstNull(nodep->propp());
        }
        නම් (!VN_IS(nodep, Restrict)) {
            puts(") begin\n");
            iterateAndNextConstNull(nodep->passsp());
            puts("end\n");
        } නැතහොත් {
            puts(");\n");
        }

        නම් (const AstAssert* const assertp = VN_CAST(nodep, Assert)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        } නැතහොත් නම් (const AstAssertIntrinsic* const assertp = VN_CAST(nodep, AssertIntrinsic)) {
            puts("else begin\n");
            iterateAndNextConstNull(assertp->failsp());
            puts("end\n");
        }
    }
    void visit(AstAssocArrayDType* nodep) override {
        නම් (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } නැතහොත් {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = false;
            puts("[");
            iterateConst(nodep->keyDTypep());
            puts("]");
            m_arrayPost = true;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    void visit(AstBasicDType* nodep) override {
        නම් (m_arrayPost) ආපසු;
        putfs(nodep, nodep->prettyName());
        නම් (nodep->isSigned() && !nodep->keyword().isDouble()) putfs(nodep, " signed");
        // Do not emit ranges for integer atoms.
        නම් (nodep->keyword().isIntNumeric() && !nodep->keyword().isBitLogic()) ආපසු;
        emitPacked();
        නම් (nodep->rangep()) {
            puts(" ");
            iterateAndNextConstNull(nodep->rangep());
            puts(" ");
        } නැතහොත් නම් (nodep->isRanged()) {
            puts(" [");
            puts(cvtToStr(nodep->left()));
            puts(":");
            puts(cvtToStr(nodep->right()));
            puts("] ");
        }
    }
    void visit(AstConstDType* nodep) override {
        නම් (m_arrayPost) ආපසු;
        putfs(nodep, "const ");
        iterateConst(nodep->subDTypep());
    }
    void visit(AstDynArrayDType* nodep) override {
        නම් (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } නැතහොත් {
            puts("[]");
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    void visit(AstEnumDType* nodep) override {
        නම් (m_arrayPost) ආපසු;
        putfs(nodep, "enum ");
        iterateConst(nodep->subDTypep());
        puts("{\n");
        iterateAndNextConstNull(nodep->itemsp());
        puts("}");
    }
    void visit(AstEnumItemRef* nodep) override {
        නම් (AstNodeModule* const classOrPackagep = nodep->classOrPackagep()) {
            putfs(nodep, classOrPackagep->prettyName());
            puts("::");
        }
        putfs(nodep, nodep->name());
    }
    void visit(AstEnumItem* nodep) override {
        putfs(nodep, nodep->name());
        iterateConstNull(nodep->rangep());
        puts(" = ");
        iterateConstNull(nodep->valuep());
        නම් (nodep->nextp()) puts(",");
        puts("\n");
    }
    void visit(AstNodeArrayDType* nodep) override {
        නම් (!m_arrayPost) {
            නම් (VN_IS(nodep, PackArrayDType)) {
                // Unpacked ranges handled in BasicDType, as they print "backwards"
                m_packedps.push_back(nodep);
            }
            iterateConst(nodep->subDTypep());
        } නැතහොත් {
            නම් (VN_IS(nodep, UnpackArrayDType)) {
                VL_RESTORER(m_arrayPost);
                m_arrayPost = false;
                iterateAndNextConstNull(nodep->rangep());
                m_arrayPost = true;
            }
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    void visit(AstIfaceRefDType* nodep) override {
        නම් (m_arrayPost) {
            puts(" (");
            නම් (nodep->cellp()) {
                iterateConst(nodep->cellp());
            } නැතහොත් {
                puts("????");
            }
            puts(")");
            ආපසු;
        }
        puts(nodep->ifaceName());
    }
    void visit(AstRefDType* nodep) override {
        නම් (nodep->subDTypep()) {
            iterateConst(nodep->skipRefp());
        } නැතහොත් {
            puts("\n???? // "s + nodep->prettyTypeName() + " -> UNLINKED\n");
        }
    }
    void visit(AstClassRefDType* nodep) override {
        UASSERT_OBJ(nodep->classp(), nodep, "AstClassRefDType not linked");
        putfs(nodep, EmitCUtil::prefixNameProtect(nodep->classp()));
    }
    void visit(AstRequireDType* nodep) override { iterateConst(nodep->lhsp()); }
    void visit(AstModport* nodep) override {
        puts(nodep->verilogKwd());
        puts(" ");
        puts(nodep->prettyName());
        puts(" (\n");
        නම් (nodep->varsp()) {
            iterateConst(nodep->varsp());
        } නැතහොත් {
            puts("????");
        }
        puts(");\n");
    }
    void visit(AstModportVarRef* nodep) override {
        puts(nodep->direction().verilogKwd());
        puts(" ");
        නම් (nodep->varp()) {
            VL_RESTORER(m_suppressVarSemi);
            m_suppressVarSemi = true;
            iterateConst(nodep->varp());
        } නැතහොත් {
            puts(nodep->prettyName());
        }
        නම් (nodep->nextp()) puts(", ");
    }
    void visit(AstNodeUOrStructDType* nodep) override {
        නම් (m_arrayPost) ආපසු;
        puts(nodep->verilogKwd() + " ");
        නම් (nodep->packed()) puts("packed ");
        {
            puts("{\n");
            VL_RESTORER_CLEAR(m_packedps);
            සඳහා (AstMemberDType* itemp = nodep->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                iterateConst(itemp);
            }
            puts("}");
        }
        emitPacked();
    }
    void visit(AstMemberDType* nodep) override {
        නම් (m_arrayPost) ආපසු;
        iterateConst(nodep->subDTypep());
        puts(" ");
        puts(nodep->name());
        puts(";\n");
    }
    void visit(AstQueueDType* nodep) override {
        නම් (!m_arrayPost) {
            iterateConst(nodep->subDTypep());
        } නැතහොත් {
            VL_RESTORER(m_arrayPost);
            m_arrayPost = false;
            puts("[$");
            නම් (nodep->boundp()) {
                puts(":");
                iterateConst(nodep->boundp());
            }
            puts("]");
            m_arrayPost = true;
            iterateConst(nodep->subDTypep());  // For post's key
        }
    }
    void visit(AstNodeFTaskRef* nodep) override {
        නම් (nodep->dotted() != "") {
            putfs(nodep, nodep->dotted());
            puts(".");
            puts(nodep->prettyName());
        } නැතහොත් {
            putfs(nodep, nodep->prettyName());
        }
        නම් (!VN_IS(nodep->taskp(), Property)) {
            puts("(");
            iterateAndNextConstNull(nodep->argsp());
            puts(")");
            iterateConstNull(nodep->withp());
        }
    }
    void visit(AstCCall* nodep) override {
        puts(nodep->funcp()->name());
        puts("(");
        iterateAndNextConstNull(nodep->argsp());
        puts(")");
    }
    void visit(AstArg* nodep) override { iterateAndNextConstNull(nodep->exprp()); }
    void visit(AstWith* nodep) override {
        putfs(nodep, " with (");
        iterateConstNull(nodep->exprp());
        puts(") ");
    }
    void visit(AstLambdaArgRef* nodep) override { putfs(nodep, nodep->name()); }
    void visit(AstPrintTimeScale* nodep) override {
        puts(nodep->verilogKwd());
        puts(";\n");
    }
    void visit(AstPropSpec* nodep) override {
        නම් (!VN_IS(nodep->propp(), FuncRef)) {
            // Same dumping as in AstSenTree
            putfs(nodep, "@(");
            සඳහා (AstNode* expp = nodep->sensesp(); expp; expp = expp->nextp()) {
                iterateConst(expp);
                නම් (expp->nextp()) putqs(expp->nextp(), " or ");
            }
            puts(")");
        }
        නම් (nodep->disablep()) {
            puts(" disable iff ");
            iterateConst(nodep->disablep());
        }

        puts(" ");
        නම් (nodep->propStrength() != VPropStrength::DEFAULT) {
            puts(nodep->propStrength().ascii());
            puts("(");
        }
        iterateConstNull(nodep->propp());
        නම් (nodep->propStrength() != VPropStrength::DEFAULT) puts(")");
        puts("\n");
    }
    void visit(AstPExpr* nodep) override { iterateConst(nodep->bodyp()); }
    void visit(AstAbortOn* nodep) override {
        puts(nodep->kind().ascii());
        puts(" (");
        iterateConst(nodep->condp());
        puts(") ");
        iterateConst(nodep->propp());
    }
    void visit(AstSClocked* nodep) override {
        puts("@(");
        iterateConst(nodep->sensesp());
        puts(") ");
        iterateConst(nodep->exprp());
        puts("\n");
    }
    void visit(AstPropAlways* nodep) override {
        puts(nodep->isStrong() ? "s_always" : "always");
        නම් (!VN_IS(nodep->loBoundp(), Unbounded) || !VN_IS(nodep->hiBoundp(), Unbounded)) {
            puts(" [");
            iterateConst(nodep->loBoundp());
            puts(":");
            iterateConst(nodep->hiBoundp());
            puts("]");
        }
        puts(" ");
        iterateConst(nodep->propp());
    }
    void visit(AstSExpr* nodep) override {
        iterateConstNull(nodep->preExprp());
        {
            VL_RESTORER(m_suppressSemi);
            m_suppressSemi = true;
            iterateConst(nodep->delayp());
        }
        iterateConst(nodep->exprp());
    }
    // Terminals
    void visit(AstVarRef* nodep) override {
        නම් (nodep->varScopep()) {
            putfs(nodep, nodep->varScopep()->prettyName());
        } නැතහොත් {
            නම් (nodep->varp()) {
                නම් (nodep->selfPointer().isEmpty()) {
                    putfs(nodep, nodep->varp()->prettyName());
                } නැතහොත් {
                    putfs(nodep, nodep->selfPointer().asString());
                    putfs(nodep, "->");
                    puts(nodep->varp()->prettyName());
                }
            } නැතහොත් {
                putfs(nodep, nodep->name());
            }
        }
    }
    void visit(AstVarXRef* nodep) override {
        putfs(nodep, nodep->prettyName(nodep->dotted()));
        puts(".");
        නම් (nodep->varp()) {
            puts(nodep->varp()->prettyName());
        } නැතහොත් {
            puts(nodep->prettyName());
        }
    }
    void visit(AstConst* nodep) override { putfs(nodep, nodep->num().ascii(m_prefixed, true)); }
    void visit(AstUnbounded* nodep) override { emitVerilogFormat(nodep, nodep->emitVerilog()); }

    // Just iterate
    void visit(AstTopScope* nodep) override { iterateChildrenConst(nodep); }
    void visit(AstScope* nodep) override { iterateChildrenConst(nodep); }
    void visit(AstVar* nodep) override {
        නම් (nodep->isIO()) {
            putfs(nodep, nodep->verilogKwd());
            puts(" ");
        }
        VL_RESTORER(m_arrayPost);
        m_arrayPost = false;
        iterateConstNull(nodep->dtypep());  // Dtype part before identifier
        puts(" ");
        puts(nodep->prettyName());
        m_arrayPost = true;
        iterateConstNull(nodep->dtypep());  // Dtype part after identifier
        puts(m_suppressVarSemi ? "\n" : ";\n");
    }
    void visit(AstActive* nodep) override {
        VL_RESTORER(m_sentreep);
        m_sentreep = nodep->sentreep();
        iterateAndNextConstNull(nodep->stmtsp());
    }
    void visit(AstDelay* nodep) override {
        puts("");  // this is for proper alignment
        නම් (nodep->isCycleDelay()) {
            puts("##");
        } නැතහොත් {
            puts("#");
        }
        VL_RESTORER(m_prefixed);
        m_prefixed = false;
        නම් (AstNodeExpr* const fallDelayp = nodep->fallDelay()) {
            puts("(");
            iterateConst(nodep->lhsp());
            puts(", ");
            iterateConst(fallDelayp);
            puts(")");
        } නැතහොත් {
            iterateConst(nodep->lhsp());
        }
        නම් (!m_suppressSemi) {
            puts(";\n");
        } නැතහොත් {
            puts(" ");
        }
        iterateAndNextConstNull(nodep->stmtsp());
    }
    void visit(AstWait* nodep) override {
        puts("wait(");
        iterateConst(nodep->condp());
        puts(")");
        නම් (nodep->stmtsp()) {
            puts(" ");
            iterateAndNextConstNull(nodep->stmtsp());
        } නැතහොත් {
            puts(";\n");
        }
    }
    void visit(AstCAwait* nodep) override {
        AstCMethodHard* methodp = VN_CAST(nodep->exprp(), CMethodHard);
        UASSERT_OBJ(methodp, nodep, "AstCAwait expression must be an AstCMethodHard");
        puts("");  // this is for proper alignment
        puts("#");
        iterateConst(methodp->pinsp());
    }
    void visit(AstParseRef* nodep) override { puts(nodep->prettyName()); }
    void visit(AstVarScope*) override {}
    void visit(AstTraceDecl*) override {}
    void visit(AstTraceInc*) override {}
    // NOPs
    void visit(AstPragma*) override {}
    void visit(AstStmtPragma*) override {}
    void visit(AstCell*) override {}  // Handled outside the Visit class
    // Default
    void visit(AstNode* nodep) override {
        puts("\n???? // "s + nodep->prettyTypeName() + "\n");
        iterateChildrenConst(nodep);
        // Not v3fatalSrc so we keep processing
        නම් (!m_suppressUnknown) {
            nodep->v3error(
                "Internal: Unknown node type reached emitter: " << nodep->prettyTypeName());
        }
    }

public:
    explicit EmitVBaseVisitorConst(bool suppressUnknown)
        : m_suppressUnknown{suppressUnknown} {}
    ~EmitVBaseVisitorConst() override = default;
};

//######################################################################
// Emit to an output file

class EmitVFileVisitor final : public EmitVBaseVisitorConst {
    // STATE
    V3OutVFile& m_of;  // The output file
    // METHODS
    void putsNoTracking(const string& str) override { m_of.putsNoTracking(str); }
    void puts(const string& str) override { m_of.puts(str); }
    void putbs(const string& str) override { m_of.putbs(str); }
    void putfs(AstNode*, const string& str) override { putbs(str); }
    void putqs(AstNode*, const string& str) override { putbs(str); }

public:
    EmitVFileVisitor(AstNode* nodep, V3OutVFile& of, bool suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_of{of} {
        iterateConst(nodep);
    }
    ~EmitVFileVisitor() override = default;
};

//######################################################################
// Emit to a stream (perhaps stringstream)

class EmitVStreamVisitor final : public EmitVBaseVisitorConst {
    // STATE
    V3OutStream m_os;  // The output stream formatter
    const bool m_tracking;  // Use line tracking
    // METHODS
    void putsNoTracking(const string& str) override { m_os.putsNoTracking(str); }
    void puts(const string& str) override {
        m_tracking ? m_os.puts(str) : m_os.putsNoTracking(str);
    }
    void putbs(const string& str) override {
        m_tracking ? m_os.putbs(str) : m_os.putsNoTracking(str);
    }
    void putfs(AstNode*, const string& str) override { putbs(str); }
    void putqs(AstNode*, const string& str) override { putbs(str); }

public:
    EmitVStreamVisitor(const AstNode* nodep, std::ostream& os, bool tracking, bool suppressUnknown)
        : EmitVBaseVisitorConst{suppressUnknown}
        , m_os{os, V3OutFormatter::LA_VERILOG}
        , m_tracking{tracking} {
        iterateConst(const_cast<AstNode*>(nodep));
    }
    ~EmitVStreamVisitor() override = default;
};

//######################################################################
// EmitV class functions

void V3EmitV::verilogForTree(const AstNode* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ false, false}; }
}

void V3EmitV::debugVerilogForTree(const AstNode* nodep, std::ostream& os) {
    { EmitVStreamVisitor{nodep, os, /* tracking: */ true, true}; }
}

std::string V3EmitV::debugVerilogForTree(const AstNode* nodep) {
    std::stringstream ss;
    debugVerilogForTree(nodep, ss);
    ආපසු ss.str();
}

void V3EmitV::emitvFiles() {
    UINFO(2, __FUNCTION__ << ":");
    සඳහා (AstNodeFile* filep = v3Global.rootp()->filesp(); filep;
         filep = VN_AS(filep->nextp(), NodeFile)) {
        AstVFile* const vfilep = VN_CAST(filep, VFile);
        නම් (vfilep && vfilep->tblockp()) {
            V3OutVFile of{vfilep->name()};
            of.puts("// DESCRIPTION: Verilator generated Verilog\n");
            EmitVFileVisitor{vfilep->tblockp(), of, false};
        }
    }
}

void V3EmitV::debugEmitV(const string& filename) {
    UINFO(2, __FUNCTION__ << ":");
    V3OutVFile of{filename};
    EmitVFileVisitor{v3Global.rootp(), of, true};
}
