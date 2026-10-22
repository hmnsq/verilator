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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Cfg.h"
#அந்தர்பாவ "V3EmitV.h"

#அந்தர்பாவ <deque>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு CfgBuilder final : பொது VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{புதிய CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    வெறுமை addStmt(Ast_முனை_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    வெறுமை nonRepresentable(Ast_முனை_Stmt*) {
        எனில் (!m_cfgp) திரும்பு;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    வெறுமை simpleStatement(Ast_முனை_Stmt* nodep, தருக்க representable = உண்மை) {
        எனில் (!m_cfgp) திரும்பு;
        // If non-representable, reset graph
        எனில் (!representable) {
            m_cfgp.reset();
            திரும்பு;
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
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!m_cfgp) திரும்பு;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    வெறுமை visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    வெறுமை visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    வெறுமை visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    வெறுமை visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    வெறுமை visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    வெறுமை visit(AstComment*) override {}  // ignore entirely
    வெறுமை visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    வெறுமை visit(AstFinish* nodep) override { simpleStatement(nodep); }
    வெறுமை visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override { simpleStatement(nodep); }
    வெறுமை visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    வெறுமை visit(AstIf* nodep) override {
        எனில் (!m_cfgp) திரும்பு;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* மாறிலி thenBBp = m_cfgp->addBlock();
        CfgBlock* மாறிலி elseBBp = m_cfgp->addBlock();
        CfgBlock* மாறிலி contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        எனில் (!m_cfgp) திரும்பு;
        எனில் (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        எனில் (!m_cfgp) திரும்பு;
        எனில் (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    வெறுமை visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        எனில் (!m_cfgp) திரும்பு;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* மாறிலி contBBp = m_cfgp->addBlock();
        மாறிலி தருக்க newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* மாறிலி bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        எனில் (!m_cfgp) திரும்பு;
        எனில் (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        எனில் (!m_cfgp) திரும்பு;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* மாறிலி contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    வெறுமை visit(AstJumpBlock* nodep) override {
        எனில் (!m_cfgp) திரும்பு;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* மாறிலி contBBp = m_cfgp->addBlock();
        மாறிலி தருக்க newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        எனில் (!m_cfgp) திரும்பு;
        எனில் (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    வெறுமை visit(AstJumpGo* nodep) override {
        எனில் (!m_cfgp) திரும்பு;

        // Non-representable if not last in statement list (V3Const will fix this later)
        எனில் (nodep->nextp()) {
            m_cfgp.reset();
            திரும்பு;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_முனை* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        எனில் (!m_cfgp) திரும்பு;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            க்கு (V3GraphVertex* மாறிலி vtxp : m_cfgp->vertices().unlinkable()) {
                எனில் (vtxp == m_cfgp->m_enterp) தொடர்;
                எனில் (vtxp == m_cfgp->m_exitp) தொடர்;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                எனில் (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            போது (!unreachableps.empty()) {
                V3GraphVertex* மாறிலி vtxp = unreachableps.back();
                unreachableps.எடு_back();
                க்கு (மாறிலி V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    எனில் (edge.உச்சி()->inSize1()) unreachableps.emplace_back(edge.உச்சி());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        எனில் (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        எனில் (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

பொது:
    நிலையான std::unique_ptr<CfgGraph> apply(Ast_முனை* stmtsp) {
        திரும்பு std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_முனை* stmtsp) { திரும்பு CfgBuilder::apply(stmtsp); }
