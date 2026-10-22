// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Control flow graph (CFG) builder
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// Control flow graph (CFG) builder
//
//*************************************************************************

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Cfg.h"
#অন্তর্ভাব "V3EmitV.h"

#অন্তর্ভাব <deque>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ CfgBuilder final : সার্বজনিক VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{নূতন CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    শূন্য addStmt(Ast_গ্রন্থি_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    শূন্য nonRepresentable(Ast_গ্রন্থি_Stmt*) {
        যদি (!m_cfgp) প্রত্যাগম;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    শূন্য simpleStatement(Ast_গ্রন্থি_Stmt* nodep, তার্কিক representable = সত্য) {
        যদি (!m_cfgp) প্রত্যাগম;
        // If non-representable, reset graph
        যদি (!representable) {
            m_cfgp.reset();
            প্রত্যাগম;
        }
        // Just add to current block
        addStmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!m_cfgp) প্রত্যাগম;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    শূন্য visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    শূন্য visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    শূন্য visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    শূন্য visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    শূন্য visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    শূন্য visit(AstComment*) override {}  // ignore entirely
    শূন্য visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    শূন্য visit(AstFinish* nodep) override { simpleStatement(nodep); }
    শূন্য visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override { simpleStatement(nodep); }
    শূন্য visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    শূন্য visit(AstIf* nodep) override {
        যদি (!m_cfgp) প্রত্যাগম;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* ধ্রুব thenBBp = m_cfgp->addBlock();
        CfgBlock* ধ্রুব elseBBp = m_cfgp->addBlock();
        CfgBlock* ধ্রুব contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        যদি (!m_cfgp) প্রত্যাগম;
        যদি (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        যদি (!m_cfgp) প্রত্যাগম;
        যদি (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        যদি (!m_cfgp) প্রত্যাগম;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ধ্রুব contBBp = m_cfgp->addBlock();
        ধ্রুব তার্কিক newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* ধ্রুব bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        যদি (!m_cfgp) প্রত্যাগম;
        যদি (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (!m_cfgp) প্রত্যাগম;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* ধ্রুব contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        যদি (!m_cfgp) প্রত্যাগম;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ধ্রুব contBBp = m_cfgp->addBlock();
        ধ্রুব তার্কিক newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        যদি (!m_cfgp) প্রত্যাগম;
        যদি (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    শূন্য visit(AstJumpGo* nodep) override {
        যদি (!m_cfgp) প্রত্যাগম;

        // Non-representable if not last in statement list (V3Const will fix this later)
        যদি (nodep->nextp()) {
            m_cfgp.reset();
            প্রত্যাগম;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_গ্রন্থি* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        যদি (!m_cfgp) প্রত্যাগম;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            প্রতি (V3GraphVertex* ধ্রুব vtxp : m_cfgp->vertices().unlinkable()) {
                যদি (vtxp == m_cfgp->m_enterp) অনুবর্ত;
                যদি (vtxp == m_cfgp->m_exitp) অনুবর্ত;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                যদি (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            যাবত্ (!unreachableps.empty()) {
                V3GraphVertex* ধ্রুব vtxp = unreachableps.back();
                unreachableps.বের_করো_back();
                প্রতি (ধ্রুব V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    যদি (edge.শীর্ষ()->inSize1()) unreachableps.emplace_back(edge.শীর্ষ());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        যদি (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        যদি (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

সার্বজনিক:
    স্থির std::unique_ptr<CfgGraph> apply(Ast_গ্রন্থি* stmtsp) {
        প্রত্যাগম std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_গ্রন্থি* stmtsp) { প্রত্যাগম CfgBuilder::apply(stmtsp); }
