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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Cfg.h"
#അന്തര്ഭാവ "V3EmitV.h"

#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം CfgBuilder final : പൊതു VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{പുതിയ CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    ശൂന്യം addStmt(Ast_കണ്ണി_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    ശൂന്യം nonRepresentable(Ast_കണ്ണി_Stmt*) {
        എങ്കിൽ (!m_cfgp) തിരികെ;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    ശൂന്യം simpleStatement(Ast_കണ്ണി_Stmt* nodep, തർക്കം representable = സത്യം) {
        എങ്കിൽ (!m_cfgp) തിരികെ;
        // If non-representable, reset graph
        എങ്കിൽ (!representable) {
            m_cfgp.reset();
            തിരികെ;
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
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!m_cfgp) തിരികെ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    ശൂന്യം visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    ശൂന്യം visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    ശൂന്യം visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    ശൂന്യം visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ശൂന്യം visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ശൂന്യം visit(AstComment*) override {}  // ignore entirely
    ശൂന്യം visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    ശൂന്യം visit(AstFinish* nodep) override { simpleStatement(nodep); }
    ശൂന്യം visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override { simpleStatement(nodep); }
    ശൂന്യം visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    ശൂന്യം visit(AstIf* nodep) override {
        എങ്കിൽ (!m_cfgp) തിരികെ;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* സ്ഥിരം thenBBp = m_cfgp->addBlock();
        CfgBlock* സ്ഥിരം elseBBp = m_cfgp->addBlock();
        CfgBlock* സ്ഥിരം contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        എങ്കിൽ (!m_cfgp) തിരികെ;
        എങ്കിൽ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        എങ്കിൽ (!m_cfgp) തിരികെ;
        എങ്കിൽ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        എങ്കിൽ (!m_cfgp) തിരികെ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* സ്ഥിരം contBBp = m_cfgp->addBlock();
        സ്ഥിരം തർക്കം newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* സ്ഥിരം bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        എങ്കിൽ (!m_cfgp) തിരികെ;
        എങ്കിൽ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        എങ്കിൽ (!m_cfgp) തിരികെ;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* സ്ഥിരം contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    ശൂന്യം visit(AstJumpBlock* nodep) override {
        എങ്കിൽ (!m_cfgp) തിരികെ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* സ്ഥിരം contBBp = m_cfgp->addBlock();
        സ്ഥിരം തർക്കം newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        എങ്കിൽ (!m_cfgp) തിരികെ;
        എങ്കിൽ (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ശൂന്യം visit(AstJumpGo* nodep) override {
        എങ്കിൽ (!m_cfgp) തിരികെ;

        // Non-representable if not last in statement list (V3Const will fix this later)
        എങ്കിൽ (nodep->nextp()) {
            m_cfgp.reset();
            തിരികെ;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_കണ്ണി* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        എങ്കിൽ (!m_cfgp) തിരികെ;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_cfgp->vertices().unlinkable()) {
                എങ്കിൽ (vtxp == m_cfgp->m_enterp) തുടരുക;
                എങ്കിൽ (vtxp == m_cfgp->m_exitp) തുടരുക;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                എങ്കിൽ (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            ആകുമ്പോൾ (!unreachableps.empty()) {
                V3GraphVertex* സ്ഥിരം vtxp = unreachableps.back();
                unreachableps.എടുക്കുക_back();
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    എങ്കിൽ (edge.മുകൾ()->inSize1()) unreachableps.emplace_back(edge.മുകൾ());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        എങ്കിൽ (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        എങ്കിൽ (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

പൊതു:
    സ്ഥിതം std::unique_ptr<CfgGraph> apply(Ast_കണ്ണി* stmtsp) {
        തിരികെ std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_കണ്ണി* stmtsp) { തിരികെ CfgBuilder::apply(stmtsp); }
