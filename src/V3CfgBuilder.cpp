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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Cfg.h"
#ضمن "V3EmitV.h"

#ضمن <deque>
#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

מחלקה CfgBuilder final : عام VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{جديد CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    فراغ addStmt(AstNodeStmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    فراغ nonRepresentable(AstNodeStmt*) {
        אם (!m_cfgp) החזר;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    فراغ simpleStatement(AstNodeStmt* nodep, منطقي representable = אמת) {
        אם (!m_cfgp) החזר;
        // If non-representable, reset graph
        אם (!representable) {
            m_cfgp.reset();
            החזר;
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
    فراغ visit(AstNode* nodep) override {
        אם (!m_cfgp) החזר;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    فراغ visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    فراغ visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    فراغ visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    فراغ visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    فراغ visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    فراغ visit(AstComment*) override {}  // ignore entirely
    فراغ visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    فراغ visit(AstFinish* nodep) override { simpleStatement(nodep); }
    فراغ visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    فراغ visit(AstStmtExpr* nodep) override { simpleStatement(nodep); }
    فراغ visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    فراغ visit(AstIf* nodep) override {
        אם (!m_cfgp) החזר;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* ثابت thenBBp = m_cfgp->addBlock();
        CfgBlock* ثابت elseBBp = m_cfgp->addBlock();
        CfgBlock* ثابت contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        אם (!m_cfgp) החזר;
        אם (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        אם (!m_cfgp) החזר;
        אם (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        אם (!m_cfgp) החזר;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ثابت contBBp = m_cfgp->addBlock();
        ثابت منطقي newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* ثابت bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        אם (!m_cfgp) החזר;
        אם (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ visit(AstLoopTest* nodep) override {
        אם (!m_cfgp) החזר;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* ثابت contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ visit(AstJumpBlock* nodep) override {
        אם (!m_cfgp) החזר;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ثابت contBBp = m_cfgp->addBlock();
        ثابت منطقي newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        אם (!m_cfgp) החזר;
        אם (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ visit(AstJumpGo* nodep) override {
        אם (!m_cfgp) החזר;

        // Non-representable if not last in statement list (V3Const will fix this later)
        אם (nodep->nextp()) {
            m_cfgp.reset();
            החזר;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(AstNode* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        אם (!m_cfgp) החזר;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            עבור (V3GraphVertex* ثابت vtxp : m_cfgp->vertices().unlinkable()) {
                אם (vtxp == m_cfgp->m_enterp) استمر;
                אם (vtxp == m_cfgp->m_exitp) استمر;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                אם (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            כלעוד (!unreachableps.empty()) {
                V3GraphVertex* ثابت vtxp = unreachableps.back();
                unreachableps.pop_back();
                עבור (ثابت V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    אם (edge.top()->inSize1()) unreachableps.emplace_back(edge.top());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        אם (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        אם (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

عام:
    ساكن std::unique_ptr<CfgGraph> apply(AstNode* stmtsp) {
        החזר std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(AstNode* stmtsp) { החזר CfgBuilder::apply(stmtsp); }
