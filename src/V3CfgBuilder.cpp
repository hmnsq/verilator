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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Cfg.h"
#अन्तर्भाव "V3EmitV.h"

#अन्तर्भाव <deque>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग CfgBuilder final : सार्वजनिक VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<CfgGraph> m_cfgp{नवीन CfgGraph};
    // Current basic block to add statements to
    CfgBlock* m_currBBp = nullptr;
    // Continuation block for given JumpBlock
    std::unordered_map<AstJumpBlock*, CfgBlock*> m_jumpBlockContp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, CfgBlock*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    शून्य addStmt(Ast_गाठ_Stmt* nodep) { m_currBBp->m_stmtps.emplace_back(nodep); }

    // Used to handle statements not representable in the CFG
    शून्य nonRepresentable(Ast_गाठ_Stmt*) {
        जर (!m_cfgp) परत;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    शून्य simpleStatement(Ast_गाठ_Stmt* nodep, तार्किक representable = सत्य) {
        जर (!m_cfgp) परत;
        // If non-representable, reset graph
        जर (!representable) {
            m_cfgp.reset();
            परत;
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
    शून्य visit(Ast_गाठ* nodep) override {
        जर (!m_cfgp) परत;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    शून्य visit(AstAssignDly* nodep) override { nonRepresentable(nodep); }
    शून्य visit(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    शून्य visit(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    शून्य visit(AstAssign* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    शून्य visit(AstAssignW* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    शून्य visit(AstComment*) override {}  // ignore entirely
    शून्य visit(AstDisplay* nodep) override { simpleStatement(nodep); }
    शून्य visit(AstFinish* nodep) override { simpleStatement(nodep); }
    शून्य visit(AstFinishFork* nodep) override { simpleStatement(nodep); }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override { simpleStatement(nodep); }
    शून्य visit(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    शून्य visit(AstIf* nodep) override {
        जर (!m_cfgp) परत;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create then/else/continuation blocks
        CfgBlock* स्थिरांक thenBBp = m_cfgp->addBlock();
        CfgBlock* स्थिरांक elseBBp = m_cfgp->addBlock();
        CfgBlock* स्थिरांक contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, thenBBp);
        m_cfgp->addUntknEdge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterateAndNextConstNull(nodep->thensp());
        जर (!m_cfgp) परत;
        जर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterateAndNextConstNull(nodep->elsesp());
        जर (!m_cfgp) परत;
        जर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        जर (!m_cfgp) परत;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* स्थिरांक contBBp = m_cfgp->addBlock();
        स्थिरांक तार्किक newEntry = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstLoop visited twice");

        // Create the body block
        CfgBlock* स्थिरांक bodyBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterateAndNextConstNull(nodep->stmtsp());
        जर (!m_cfgp) परत;
        जर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य visit(AstLoopTest* nodep) override {
        जर (!m_cfgp) परत;

        // Add terminator statement to current block - semantically the condition check only ...
        addStmt(nodep);

        // Create continuation blocks
        CfgBlock* स्थिरांक contBBp = m_cfgp->addBlock();
        m_cfgp->addTakenEdge(m_currBBp, contBBp);
        m_cfgp->addUntknEdge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य visit(AstJumpBlock* nodep) override {
        जर (!m_cfgp) परत;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        CfgBlock* स्थिरांक contBBp = m_cfgp->addBlock();
        स्थिरांक तार्किक newEntry = m_jumpBlockContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(newEntry, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterateAndNextConstNull(nodep->stmtsp());
        जर (!m_cfgp) परत;
        जर (m_currBBp) m_cfgp->addTakenEdge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य visit(AstJumpGo* nodep) override {
        जर (!m_cfgp) परत;

        // Non-representable if not last in statement list (V3Const will fix this later)
        जर (nodep->nextp()) {
            m_cfgp.reset();
            परत;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->addTakenEdge(m_currBBp, m_jumpBlockContp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = nullptr;
    }

    // CONSTRUCTOR
    explicit CfgBuilder(Ast_गाठ* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->addBlock();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterateAndNextConstNull(stmtsp);
        // If failed, stop now
        जर (!m_cfgp) परत;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::vector<V3GraphVertex*> unreachableps;
            साठी (V3GraphVertex* स्थिरांक vtxp : m_cfgp->vertices().unlinkable()) {
                जर (vtxp == m_cfgp->m_enterp) सुरूठेवा;
                जर (vtxp == m_cfgp->m_exitp) सुरूठेवा;
                UASSERT_OBJ(!vtxp->outEmpty(), vtxp, "Block with no successor other than exit");
                जर (vtxp->inEmpty()) unreachableps.emplace_back(vtxp);
            }
            पर्यंत (!unreachableps.empty()) {
                V3GraphVertex* स्थिरांक vtxp = unreachableps.back();
                unreachableps.काढा_back();
                साठी (स्थिरांक V3GraphEdge& edge : vtxp->outEdges()) {
                    --m_cfgp->m_nEdges;
                    जर (edge.शीर्ष()->inSize1()) unreachableps.emplace_back(edge.शीर्ष());
                }
                --m_cfgp->m_nBlocks;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        जर (dumpGraphLevel() >= 9) {
            m_cfgp->rpoBlocks();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        जर (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

सार्वजनिक:
    स्थिर std::unique_ptr<CfgGraph> apply(Ast_गाठ* stmtsp) {
        परत std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_गाठ* stmtsp) { परत CfgBuilder::apply(stmtsp); }
