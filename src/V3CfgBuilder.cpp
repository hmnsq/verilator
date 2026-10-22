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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Cfg.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ CfgBuilder final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{ನೂತನ CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    ಶೂನ್ಯ addStmt(Ast_ಗಂಟು_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    ಶೂನ್ಯ nonRepresentable(Ast_ಗಂಟು_Stmt*) {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    ಶೂನ್ಯ simpleStatement(Ast_ಗಂಟು_Stmt* nodep, ತಾರ್ಕಿಕ representable = ಸತ್ಯ) {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        // If non-representable, reset graph
        ಯದಿ (!representable) {
            m_cfgp.reset();
            ಪ್ರತ್ಯಾಗಮ;
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
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    ಶೂನ್ಯ visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    ಶೂನ್ಯ visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    ಶೂನ್ಯ visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ಶೂನ್ಯ visit(AstComment*) override {}  // ignore entirely
    ಶೂನ್ಯ visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ visit(AstFinish* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    ಶೂನ್ಯ visit(AstIf* nodep) override {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* ಧ್ರುವ thenBBp = m_cfgp->addBlock();
        CfgBlock* ಧ್ರುವ elseBBp = m_cfgp->addBlock();
        CfgBlock* ಧ್ರುವ contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ಧ್ರುವ contBBp = m_cfgp->addBlock();
        ಧ್ರುವ ತಾರ್ಕಿಕ newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* ಧ್ರುವ bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* ಧ್ರುವ contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ಧ್ರುವ contBBp = m_cfgp->addBlock();
        ಧ್ರುವ ತಾರ್ಕಿಕ newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;

        // Non-representable if not last in statement list (V3Const will fix this later)
        ಯದಿ (nodep->nextp()) {
            m_cfgp.reset();
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_ಗಂಟು* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        ಯದಿ (!m_cfgp) ಪ್ರತ್ಯಾಗಮ;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_cfgp->vertices().unlinkable()) {
                ಯದಿ (vtxp == m_cfgp->m_enterp) ಅನುವರ್ತ;
                ಯದಿ (vtxp == m_cfgp->m_exitp) ಅನುವರ್ತ;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                ಯದಿ (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            ಯಾವತ್ (!unreachableps.empty()) {
                V3GraphVertex* ಧ್ರುವ vtxp = unreachableps.back();
                unreachableps.ತೆಗೆ_back();
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    ಯದಿ (edge.ತುದಿ()->inSize1()) unreachableps.emplace_back(edge.ತುದಿ());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        ಯದಿ (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        ಯದಿ (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<CfgGraph> apply(Ast_ಗಂಟು* stmtsp) {
        ಪ್ರತ್ಯಾಗಮ std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_ಗಂಟು* stmtsp) { ಪ್ರತ್ಯಾಗಮ CfgBuilder::apply(stmtsp); }
