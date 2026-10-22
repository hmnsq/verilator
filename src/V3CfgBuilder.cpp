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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Cfg.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"

#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ CfgBuilder final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{ନୂତନ CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    ଶୂନ୍ୟ addStmt(Ast_ଗଣ୍ଠି_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    ଶୂନ୍ୟ nonRepresentable(Ast_ଗଣ୍ଠି_Stmt*) {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    ଶୂନ୍ୟ simpleStatement(Ast_ଗଣ୍ଠି_Stmt* nodep, ତାର୍କିକ representable = ସତ୍ୟ) {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        // If non-representable, reset graph
        ଯଦି (!representable) {
            m_cfgp.reset();
            ଫେରନ୍ତୁ;
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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    ଶୂନ୍ୟ visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    ଶୂନ୍ୟ visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    ଶୂନ୍ୟ visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ଶୂନ୍ୟ visit(AstComment*) override {}  // ignore entirely
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    ଶୂନ୍ୟ visit(AstFinish* nodep) override { simpleStatement(nodep); }
    ଶୂନ୍ୟ visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override { simpleStatement(nodep); }
    ଶୂନ୍ୟ visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* ଅଚଳ thenBBp = m_cfgp->addBlock();
        CfgBlock* ଅଚଳ elseBBp = m_cfgp->addBlock();
        CfgBlock* ଅଚଳ contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        ଯଦି (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        ଯଦି (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ଅଚଳ contBBp = m_cfgp->addBlock();
        ଅଚଳ ତାର୍କିକ newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* ଅଚଳ bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        ଯଦି (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* ଅଚଳ contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* ଅଚଳ contBBp = m_cfgp->addBlock();
        ଅଚଳ ତାର୍କିକ newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        ଯଦି (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;

        // Non-representable if not last in statement list (V3Const will fix this later)
        ଯଦି (nodep->nextp()) {
            m_cfgp.reset();
            ଫେରନ୍ତୁ;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_ଗଣ୍ଠି* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        ଯଦି (!m_cfgp) ଫେରନ୍ତୁ;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_cfgp->vertices().unlinkable()) {
                ଯଦି (vtxp == m_cfgp->m_enterp) ଜାରି;
                ଯଦି (vtxp == m_cfgp->m_exitp) ଜାରି;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                ଯଦି (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            ଯେତେବେଳେ (!unreachableps.empty()) {
                V3GraphVertex* ଅଚଳ vtxp = unreachableps.back();
                unreachableps.କାଢ଼_back();
                ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    ଯଦି (edge.ଶୀର୍ଷ()->inSize1()) unreachableps.emplace_back(edge.ଶୀର୍ଷ());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        ଯଦି (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        ଯଦି (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::unique_ptr<CfgGraph> apply(Ast_ଗଣ୍ଠି* stmtsp) {
        ଫେରନ୍ତୁ std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_ଗଣ୍ଠି* stmtsp) { ଫେରନ୍ତୁ CfgBuilder::apply(stmtsp); }
