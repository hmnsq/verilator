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

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ CfgBuilder final : ਜਨਤਕ VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<Cfg_ਗ੍ਰਾਫ> m_cfgp{ਨਵਾਂ Cfg_ਗ੍ਰਾਫ};
    // Current basic block to add statements to
    Cfg_ਖੰਡ* m_currBBp = ਖਾਲੀ_ਸੂਚਕ;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_ਖੰਡ*, Cfg_ਖੰਡ*> m_jump_ਖੰਡ_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_ਖੰਡ*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    ਖਾਲੀ ਜੋੜੋ_Stmt(Ast_ਗੰਢ_Stmt* nodep) { m_currBBp->m_stmtps.emplace_ਪਿੱਠ(nodep); }

    // Used to handle statements not representable in the CFG
    ਖਾਲੀ nonRepresentable(Ast_ਗੰਢ_Stmt*) {
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    ਖਾਲੀ simpleStatement(Ast_ਗੰਢ_Stmt* nodep, ਤਾਰਕਿਕ representable = ਸੱਚ) {
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        // If non-representable, reset graph
        ਜੇ (!representable) {
            m_cfgp.reset();
            ਵਾਪਸ;
        }
        // Just add to current block
        ਜੋੜੋ_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override { nonRepresentable(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    ਖਾਲੀ ਦੇਖੋ(AstComment*) override {}  // ignore entirely
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override { simpleStatement(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ* nodep) override { simpleStatement(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ_Fork* nodep) override { simpleStatement(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override { simpleStatement(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        ਜੇ (!m_cfgp) ਵਾਪਸ;

        // Add terminator statement to current block - semantically the condition check only ...
        ਜੋੜੋ_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_ਖੰਡ* ਅਚਲ thenBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        Cfg_ਖੰਡ* ਅਚਲ elseBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        Cfg_ਖੰਡ* ਅਚਲ contBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, thenBBp);
        m_cfgp->ਜੋੜੋ_Untkn_ਕਿਨਾਰਾ(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        ਜੇ (m_currBBp) m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        ਜੇ (m_currBBp) m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ਜੇ (!m_cfgp) ਵਾਪਸ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_ਖੰਡ* ਅਚਲ contBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = m_loopContp.emplace(nodep, contBBp).ਦੂਜਾ;
        UASSERT_OBJ(new_ਇੰਦਰਾਜ, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_ਖੰਡ* ਅਚਲ bodyBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        ਜੇ (m_currBBp) m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        ਜੇ (!m_cfgp) ਵਾਪਸ;

        // Add terminator statement to current block - semantically the condition check only ...
        ਜੋੜੋ_Stmt(nodep);

        // Create continuation blocks
        Cfg_ਖੰਡ* ਅਚਲ contBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, contBBp);
        m_cfgp->ਜੋੜੋ_Untkn_ਕਿਨਾਰਾ(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        ਜੇ (!m_cfgp) ਵਾਪਸ;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_ਖੰਡ* ਅਚਲ contBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = m_jump_ਖੰਡ_Contp.emplace(nodep, contBBp).ਦੂਜਾ;
        UASSERT_OBJ(new_ਇੰਦਰਾਜ, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        ਜੇ (m_currBBp) m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        ਜੇ (!m_cfgp) ਵਾਪਸ;

        // Non-representable if not last in statement list (V3Const will fix this later)
        ਜੇ (nodep->nextp()) {
            m_cfgp.reset();
            ਵਾਪਸ;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(m_currBBp, m_jump_ਖੰਡ_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = ਖਾਲੀ_ਸੂਚਕ;
    }

    // CONSTRUCTOR
    ਸਪਸ਼ਟ CfgBuilder(Ast_ਗੰਢ* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->ਜੋੜੋ_ਖੰਡ();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_ਅਗਲਾ_Const_Null(stmtsp);
        // If failed, stop now
        ਜੇ (!m_cfgp) ਵਾਪਸ;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> unreachableps;
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_cfgp->vertices().unlinkable()) {
                ਜੇ (vtxp == m_cfgp->m_enterp) ਜਾਰੀ;
                ਜੇ (vtxp == m_cfgp->m_exitp) ਜਾਰੀ;
                UASSERT_OBJ(!vtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ(), vtxp, "Block with no successor other than exit");
                ਜੇ (vtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) unreachableps.emplace_ਪਿੱਠ(vtxp);
            }
            ਜਦੋਂ (!unreachableps.ਖਾਲੀ_ਥਾਵਾਂ()) {
                V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = unreachableps.ਪਿੱਠ();
                unreachableps.ਕੱਢੋ_ਪਿੱਠ();
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    --m_cfgp->m_n_ਕਿਨਾਰੇ;
                    ਜੇ (ਕਿਨਾਰਾ.ਸਿਖਰ()->ਵਿੱਚ_Size1()) unreachableps.emplace_ਪਿੱਠ(ਕਿਨਾਰਾ.ਸਿਖਰ());
                }
                --m_cfgp->m_n_ਖੰਡਾਂ;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) {
            m_cfgp->rpo_ਖੰਡਾਂ();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

ਜਨਤਕ:
    ਸਥਿਰ std::unique_ptr<Cfg_ਗ੍ਰਾਫ> apply(Ast_ਗੰਢ* stmtsp) {
        ਵਾਪਸ std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<Cfg_ਗ੍ਰਾਫ> Cfg_ਗ੍ਰਾਫ::build(Ast_ਗੰਢ* stmtsp) { ਵਾਪਸ CfgBuilder::apply(stmtsp); }
