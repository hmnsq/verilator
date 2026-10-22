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
    std::unique_ptr<Cfg_ನಕ್ಷೆ> m_cfgp{ಹೊಸ Cfg_ನಕ್ಷೆ};
    // Current basic block to add statements to
    Cfg_ಖಂಡ* m_currBBp = ಶೂನ್ಯಸೂಚಕ;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_ಖಂಡ*, Cfg_ಖಂಡ*> m_jump_ಖಂಡ_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_ಖಂಡ*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    ಶೂನ್ಯ ಕೂಡಿಸು_Stmt(Ast_ಗಂಟು_Stmt* nodep) { m_currBBp->m_stmtps.emplace_ಹಿಂಭಾಗ(nodep); }

    // Used to handle statements not representable in the CFG
    ಶೂನ್ಯ nonRepresentable(Ast_ಗಂಟು_Stmt*) {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    ಶೂನ್ಯ simpleStatement(Ast_ಗಂಟು_Stmt* nodep, ತಾರ್ಕಿಕ representable = ನಿಜ) {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        // If non-representable, reset graph
        ಆದರೆ (!representable) {
            m_cfgp.reset();
            ಹಿಂತಿರುಗಿ;
        }
        // Just add to current block
        ಕೂಡಿಸು_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override { nonRepresentable(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstComment*) override {}  // ignore entirely
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ_Fork* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override { simpleStatement(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;

        // Add terminator statement to current block - semantically the condition check only ...
        ಕೂಡಿಸು_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ thenBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ elseBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ contBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, thenBBp);
        m_cfgp->ಕೂಡಿಸು_Untkn_ಅಂಚು(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_currBBp) m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_currBBp) m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ contBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = m_loopContp.emplace(nodep, contBBp).ಎರಡನೇ;
        UASSERT_OBJ(new_ನಮೂದು, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ bodyBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_currBBp) m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;

        // Add terminator statement to current block - semantically the condition check only ...
        ಕೂಡಿಸು_Stmt(nodep);

        // Create continuation blocks
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ contBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, contBBp);
        m_cfgp->ಕೂಡಿಸು_Untkn_ಅಂಚು(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ contBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = m_jump_ಖಂಡ_Contp.emplace(nodep, contBBp).ಎರಡನೇ;
        UASSERT_OBJ(new_ನಮೂದು, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_currBBp) m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;

        // Non-representable if not last in statement list (V3Const will fix this later)
        ಆದರೆ (nodep->nextp()) {
            m_cfgp.reset();
            ಹಿಂತಿರುಗಿ;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(m_currBBp, m_jump_ಖಂಡ_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = ಶೂನ್ಯಸೂಚಕ;
    }

    // CONSTRUCTOR
    ಸ್ಪಷ್ಟ CfgBuilder(Ast_ಗಂಟು* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->ಕೂಡಿಸು_ಖಂಡ();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_ಮುಂದಿನ_Const_Null(stmtsp);
        // If failed, stop now
        ಆದರೆ (!m_cfgp) ಹಿಂತಿರುಗಿ;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*> unreachableps;
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_cfgp->vertices().unlinkable()) {
                ಆದರೆ (vtxp == m_cfgp->m_enterp) ಮುಂದುವರಿ;
                ಆದರೆ (vtxp == m_cfgp->m_exitp) ಮುಂದುವರಿ;
                UASSERT_OBJ(!vtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು(), vtxp, "Block with no successor other than exit");
                ಆದರೆ (vtxp->ಒಳ_ಖಾಲಿಗಳು()) unreachableps.emplace_ಹಿಂಭಾಗ(vtxp);
            }
            ಆಗ (!unreachableps.ಖಾಲಿಗಳು()) {
                V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = unreachableps.ಹಿಂಭಾಗ();
                unreachableps.ತೆಗೆ_ಹಿಂಭಾಗ();
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    --m_cfgp->m_n_ಅಂಚುಗಳು;
                    ಆದರೆ (ಅಂಚು.ತುದಿ()->ಒಳ_Size1()) unreachableps.emplace_ಹಿಂಭಾಗ(ಅಂಚು.ತುದಿ());
                }
                --m_cfgp->m_n_ಖಂಡಗಳು;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) {
            m_cfgp->rpo_ಖಂಡಗಳು();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<Cfg_ನಕ್ಷೆ> apply(Ast_ಗಂಟು* stmtsp) {
        ಹಿಂತಿರುಗಿ std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<Cfg_ನಕ್ಷೆ> Cfg_ನಕ್ಷೆ::build(Ast_ಗಂಟು* stmtsp) { ಹಿಂತಿರುಗಿ CfgBuilder::apply(stmtsp); }
