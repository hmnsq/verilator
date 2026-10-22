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

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Cfg.h"
#समावेश "V3EmitV.h"

#समावेश <deque>
#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी CfgBuilder final : खुला VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{नया CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    व्योम addStmt(Ast_गाँठ_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    व्योम nonRepresentable(Ast_गाँठ_Stmt*) {
        अगर (!m_cfgp) वापस;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    व्योम simpleStatement(Ast_गाँठ_Stmt* nodep, bool representable = true) {
        अगर (!m_cfgp) वापस;
        // If non-representable, reset graph
        अगर (!representable) {
            m_cfgp.reset();
            वापस;
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
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!m_cfgp) वापस;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    व्योम visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    व्योम visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    व्योम visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    व्योम visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    व्योम visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    व्योम visit(AstComment*) override {}  // ignore entirely
    व्योम visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    व्योम visit(AstFinish* nodep) override { simpleStatement(nodep); }
    व्योम visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override { simpleStatement(nodep); }
    व्योम visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    व्योम visit(AstIf* nodep) override {
        अगर (!m_cfgp) वापस;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* स्थिर thenBBp = m_cfgp->addBlock();
        CfgBlock* स्थिर elseBBp = m_cfgp->addBlock();
        CfgBlock* स्थिर contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_अग्र_Const_Null(nodep->thensp());
        अगर (!m_cfgp) वापस;
        अगर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        अगर (!m_cfgp) वापस;
        अगर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    व्योम visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        अगर (!m_cfgp) वापस;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* स्थिर contBBp = m_cfgp->addBlock();
        स्थिर bool newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* स्थिर bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        अगर (!m_cfgp) वापस;
        अगर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    व्योम visit(AstLoopTest* nodep) override {
        अगर (!m_cfgp) वापस;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* स्थिर contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    व्योम visit(AstJumpBlock* nodep) override {
        अगर (!m_cfgp) वापस;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* स्थिर contBBp = m_cfgp->addBlock();
        स्थिर bool newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        अगर (!m_cfgp) वापस;
        अगर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    व्योम visit(AstJumpGo* nodep) override {
        अगर (!m_cfgp) वापस;

        // Non-representable if not last in statement list (V3Const will fix this later)
        अगर (nodep->nextp()) {
            m_cfgp.reset();
            वापस;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_गाँठ* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_अग्र_Const_Null(stmtsp);
        // If failed, stop now
        अगर (!m_cfgp) वापस;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            क्रम (V3GraphVertex* स्थिर vtxp : m_cfgp->vertices().unlinkable()) {
                अगर (vtxp == m_cfgp->m_enterp) जारी;
                अगर (vtxp == m_cfgp->m_exitp) जारी;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                अगर (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            जबतक (!unreachableps.empty()) {
                V3GraphVertex* स्थिर vtxp = unreachableps.back();
                unreachableps.निकालो_back();
                क्रम (स्थिर V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    अगर (edge.शीर्ष()->inSize1()) unreachableps.emplace_back(edge.शीर्ष());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.लो()), vtxp);
            }
        }
        // Dump the initial graph
        अगर (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        अगर (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

खुला:
    अटल std::unique_ptr<CfgGraph> apply(Ast_गाँठ* stmtsp) {
        वापस std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_गाँठ* stmtsp) { वापस CfgBuilder::apply(stmtsp); }
