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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Cfg.h"
#અન્તર્ભાવ "V3EmitV.h"

#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ CfgBuilder final : જાહેર VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{નવું CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    શૂન્ય addStmt(Ast_ગાંઠ_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    શૂન્ય nonRepresentable(Ast_ગાંઠ_Stmt*) {
        જો (!m_cfgp) પાછા;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    શૂન્ય simpleStatement(Ast_ગાંઠ_Stmt* nodep, તાર્કિક representable = સાચું) {
        જો (!m_cfgp) પાછા;
        // If non-representable, reset graph
        જો (!representable) {
            m_cfgp.reset();
            પાછા;
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
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (!m_cfgp) પાછા;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    શૂન્ય visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    શૂન્ય visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    શૂન્ય visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    શૂન્ય visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    શૂન્ય visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    શૂન્ય visit(AstComment*) override {}  // ignore entirely
    શૂન્ય visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    શૂન્ય visit(AstFinish* nodep) override { simpleStatement(nodep); }
    શૂન્ય visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override { simpleStatement(nodep); }
    શૂન્ય visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    શૂન્ય visit(AstIf* nodep) override {
        જો (!m_cfgp) પાછા;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* અચલ thenBBp = m_cfgp->addBlock();
        CfgBlock* અચલ elseBBp = m_cfgp->addBlock();
        CfgBlock* અચલ contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        જો (!m_cfgp) પાછા;
        જો (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        જો (!m_cfgp) પાછા;
        જો (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    શૂન્ય visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        જો (!m_cfgp) પાછા;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* અચલ contBBp = m_cfgp->addBlock();
        અચલ તાર્કિક newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* અચલ bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        જો (!m_cfgp) પાછા;
        જો (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        જો (!m_cfgp) પાછા;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* અચલ contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    શૂન્ય visit(AstJumpBlock* nodep) override {
        જો (!m_cfgp) પાછા;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* અચલ contBBp = m_cfgp->addBlock();
        અચલ તાર્કિક newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        જો (!m_cfgp) પાછા;
        જો (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    શૂન્ય visit(AstJumpGo* nodep) override {
        જો (!m_cfgp) પાછા;

        // Non-representable if not last in statement list (V3Const will fix this later)
        જો (nodep->nextp()) {
            m_cfgp.reset();
            પાછા;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_ગાંઠ* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        જો (!m_cfgp) પાછા;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            માટે (V3GraphVertex* અચલ vtxp : m_cfgp->vertices().unlinkable()) {
                જો (vtxp == m_cfgp->m_enterp) ચાલુ;
                જો (vtxp == m_cfgp->m_exitp) ચાલુ;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                જો (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            જ્યારે (!unreachableps.empty()) {
                V3GraphVertex* અચલ vtxp = unreachableps.back();
                unreachableps.કાઢો_back();
                માટે (અચલ V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    જો (edge.ટોચ()->inSize1()) unreachableps.emplace_back(edge.ટોચ());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        જો (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        જો (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

જાહેર:
    સ્થિર std::unique_ptr<CfgGraph> apply(Ast_ગાંઠ* stmtsp) {
        પાછા std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_ગાંઠ* stmtsp) { પાછા CfgBuilder::apply(stmtsp); }
