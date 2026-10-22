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
    std::unique_ptr<CfgGraph> m_cfgp{नूतन CfgGraph};
    // Current basic block to add statements to
    Cfg_खण्डम्* m_currBBp = शून्यसूचकः;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_खण्डम्*, Cfg_खण्डम्*> m_jump_खण्डम्_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_खण्डम्*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    शून्य सङ्कलय_Stmt(Ast_ग्रन्थिः_Stmt* nodep) { m_currBBp->m_stmtps.emplace_पृष्ठभागः(nodep); }

    // Used to handle statements not representable in the CFG
    शून्य nonRepresentable(Ast_ग्रन्थिः_Stmt*) {
        यदि (!m_cfgp) प्रत्यागम;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    शून्य simpleStatement(Ast_ग्रन्थिः_Stmt* nodep, तार्किक representable = सत्य) {
        यदि (!m_cfgp) प्रत्यागम;
        // If non-representable, reset graph
        यदि (!representable) {
            m_cfgp.reset();
            प्रत्यागम;
        }
        // Just add to current block
        सङ्कलय_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (!m_cfgp) प्रत्यागम;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override { nonRepresentable(nodep); }
    शून्य दर्शय(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    शून्य दर्शय(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    शून्य दर्शय(Ast_नियोजय* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    शून्य दर्शय(AstComment*) override {}  // ignore entirely
    शून्य दर्शय(AstDisplay* nodep) override { simpleStatement(nodep); }
    शून्य दर्शय(AstFinish* nodep) override { simpleStatement(nodep); }
    शून्य दर्शय(AstFinishFork* nodep) override { simpleStatement(nodep); }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override { simpleStatement(nodep); }
    शून्य दर्शय(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    शून्य दर्शय(AstIf* nodep) override {
        यदि (!m_cfgp) प्रत्यागम;

        // Add terminator statement to current block - semantically the condition check only ...
        सङ्कलय_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_खण्डम्* ध्रुव thenBBp = m_cfgp->सङ्कलय_खण्डम्();
        Cfg_खण्डम्* ध्रुव elseBBp = m_cfgp->सङ्कलय_खण्डम्();
        Cfg_खण्डम्* ध्रुव contBBp = m_cfgp->सङ्कलय_खण्डम्();
        m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, thenBBp);
        m_cfgp->सङ्कलय_Untkn_Edge(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_अग्र_Const_Null(nodep->thensp());
        यदि (!m_cfgp) प्रत्यागम;
        यदि (m_currBBp) m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        यदि (!m_cfgp) प्रत्यागम;
        यदि (m_currBBp) m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य दर्शय(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        यदि (!m_cfgp) प्रत्यागम;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_खण्डम्* ध्रुव contBBp = m_cfgp->सङ्कलय_खण्डम्();
        ध्रुव तार्किक new_प्रविष्टिः = m_loopContp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(new_प्रविष्टिः, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_खण्डम्* ध्रुव bodyBBp = m_cfgp->सङ्कलय_खण्डम्();
        m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        यदि (!m_cfgp) प्रत्यागम;
        यदि (m_currBBp) m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        यदि (!m_cfgp) प्रत्यागम;

        // Add terminator statement to current block - semantically the condition check only ...
        सङ्कलय_Stmt(nodep);

        // Create continuation blocks
        Cfg_खण्डम्* ध्रुव contBBp = m_cfgp->सङ्कलय_खण्डम्();
        m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, contBBp);
        m_cfgp->सङ्कलय_Untkn_Edge(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        यदि (!m_cfgp) प्रत्यागम;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_खण्डम्* ध्रुव contBBp = m_cfgp->सङ्कलय_खण्डम्();
        ध्रुव तार्किक new_प्रविष्टिः = m_jump_खण्डम्_Contp.emplace(nodep, contBBp).second;
        UASSERT_OBJ(new_प्रविष्टिः, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        यदि (!m_cfgp) प्रत्यागम;
        यदि (m_currBBp) m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    शून्य दर्शय(AstJumpGo* nodep) override {
        यदि (!m_cfgp) प्रत्यागम;

        // Non-representable if not last in statement list (V3Const will fix this later)
        यदि (nodep->nextp()) {
            m_cfgp.reset();
            प्रत्यागम;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->सङ्कलय_गृहीतम्_Edge(m_currBBp, m_jump_खण्डम्_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = शून्यसूचकः;
    }

    // CONSTRUCTOR
    स्पष्ट CfgBuilder(Ast_ग्रन्थिः* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->सङ्कलय_खण्डम्();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_अग्र_Const_Null(stmtsp);
        // If failed, stop now
        यदि (!m_cfgp) प्रत्यागम;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::सदिशम्<V3GraphVertex*> unreachableps;
            प्रति (V3GraphVertex* ध्रुव vtxp : m_cfgp->vertices().unlinkable()) {
                यदि (vtxp == m_cfgp->m_enterp) अनुवर्त;
                यदि (vtxp == m_cfgp->m_exitp) अनुवर्त;
                UASSERT_OBJ(!vtxp->बहिर्गमनम्_रिक्तानि(), vtxp, "Block with no successor other than exit");
                यदि (vtxp->अन्तर्_रिक्तानि()) unreachableps.emplace_पृष्ठभागः(vtxp);
            }
            यावत् (!unreachableps.रिक्तानि()) {
                V3GraphVertex* ध्रुव vtxp = unreachableps.पृष्ठभागः();
                unreachableps.निष्कासय_पृष्ठभागः();
                प्रति (ध्रुव V3GraphEdge& edge : vtxp->बहिर्गमनम्_धाराः()) {
                    --m_cfgp->m_n_धाराः;
                    यदि (edge.शिखरम्()->अन्तर्_Size1()) unreachableps.emplace_पृष्ठभागः(edge.शिखरम्());
                }
                --m_cfgp->m_n_खण्डानि;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        यदि (dumpGraphLevel() >= 9) {
            m_cfgp->rpo_खण्डानि();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        यदि (dumpGraphLevel() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

सार्वजनिक:
    स्थिर std::unique_ptr<CfgGraph> apply(Ast_ग्रन्थिः* stmtsp) {
        प्रत्यागम std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<CfgGraph> CfgGraph::build(Ast_ग्रन्थिः* stmtsp) { प्रत्यागम CfgBuilder::apply(stmtsp); }
