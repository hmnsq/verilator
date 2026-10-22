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

صنف CfgBuilder final : عام VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<Cfg_گراف> m_cfgp{جديد Cfg_گراف};
    // Current basic block to add statements to
    Cfg_بلاڪ* m_currBBp = خالي_اشارو;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_بلاڪ*, Cfg_بلاڪ*> m_jump_بلاڪ_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_بلاڪ*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    فراغ جوڙيو_Stmt(Ast_ڳنڍ_Stmt* nodep) { m_currBBp->m_stmtps.emplace_پٺ(nodep); }

    // Used to handle statements not representable in the CFG
    فراغ nonRepresentable(Ast_ڳنڍ_Stmt*) {
        إذا (!m_cfgp) أرجع;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    فراغ simpleStatement(Ast_ڳنڍ_Stmt* nodep, منطقي representable = سچ) {
        إذا (!m_cfgp) أرجع;
        // If non-representable, reset graph
        إذا (!representable) {
            m_cfgp.reset();
            أرجع;
        }
        // Just add to current block
        جوڙيو_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (!m_cfgp) أرجع;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override { nonRepresentable(nodep); }
    فراغ گهمو(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    فراغ گهمو(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    فراغ گهمو(Ast_سونپيو* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    فراغ گهمو(Ast_سونپيو_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    فراغ گهمو(AstComment*) override {}  // ignore entirely
    فراغ گهمو(AstDisplay* nodep) override { simpleStatement(nodep); }
    فراغ گهمو(Ast_پڄاڻي* nodep) override { simpleStatement(nodep); }
    فراغ گهمو(Ast_پڄاڻي_Fork* nodep) override { simpleStatement(nodep); }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override { simpleStatement(nodep); }
    فراغ گهمو(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    فراغ گهمو(AstIf* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Add terminator statement to current block - semantically the condition check only ...
        جوڙيو_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_بلاڪ* ثابت thenBBp = m_cfgp->جوڙيو_بلاڪ();
        Cfg_بلاڪ* ثابت elseBBp = m_cfgp->جوڙيو_بلاڪ();
        Cfg_بلاڪ* ثابت contBBp = m_cfgp->جوڙيو_بلاڪ();
        m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, thenBBp);
        m_cfgp->جوڙيو_Untkn_ڪنارو(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_ايندڙ_Const_Null(nodep->thensp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_ايندڙ_Const_Null(nodep->elsesp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ گهمو(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        إذا (!m_cfgp) أرجع;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_بلاڪ* ثابت contBBp = m_cfgp->جوڙيو_بلاڪ();
        ثابت منطقي new_داخلا = m_loopContp.emplace(nodep, contBBp).ٻيو;
        UASSERT_OBJ(new_داخلا, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_بلاڪ* ثابت bodyBBp = m_cfgp->جوڙيو_بلاڪ();
        m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ گهمو(AstLoopTest* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Add terminator statement to current block - semantically the condition check only ...
        جوڙيو_Stmt(nodep);

        // Create continuation blocks
        Cfg_بلاڪ* ثابت contBBp = m_cfgp->جوڙيو_بلاڪ();
        m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, contBBp);
        m_cfgp->جوڙيو_Untkn_ڪنارو(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_بلاڪ* ثابت contBBp = m_cfgp->جوڙيو_بلاڪ();
        ثابت منطقي new_داخلا = m_jump_بلاڪ_Contp.emplace(nodep, contBBp).ٻيو;
        UASSERT_OBJ(new_داخلا, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ گهمو(AstJumpGo* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Non-representable if not last in statement list (V3Const will fix this later)
        إذا (nodep->nextp()) {
            m_cfgp.reset();
            أرجع;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->جوڙيو_ورتل_ڪنارو(m_currBBp, m_jump_بلاڪ_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = خالي_اشارو;
    }

    // CONSTRUCTOR
    واضح CfgBuilder(Ast_ڳنڍ* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->جوڙيو_بلاڪ();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_ايندڙ_Const_Null(stmtsp);
        // If failed, stop now
        إذا (!m_cfgp) أرجع;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::ويڪٽر<V3_گراف_راس*> unreachableps;
            لكل (V3_گراف_راس* ثابت vtxp : m_cfgp->vertices().unlinkable()) {
                إذا (vtxp == m_cfgp->m_enterp) استمر;
                إذا (vtxp == m_cfgp->m_exitp) استمر;
                UASSERT_OBJ(!vtxp->ٻاهريون_خالي_جايون(), vtxp, "Block with no successor other than exit");
                إذا (vtxp->اندريون_خالي_جايون()) unreachableps.emplace_پٺ(vtxp);
            }
            طالما (!unreachableps.خالي_جايون()) {
                V3_گراف_راس* ثابت vtxp = unreachableps.پٺ();
                unreachableps.ڪڍو_پٺ();
                لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vtxp->ٻاهريون_ڪنارا()) {
                    --m_cfgp->m_n_ڪنارا;
                    إذا (ڪنارو.چوٽي()->اندريون_Size1()) unreachableps.emplace_پٺ(ڪنارو.چوٽي());
                }
                --m_cfgp->m_n_بلاڪون;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        إذا (dump_گراف_Level() >= 9) {
            m_cfgp->rpo_بلاڪون();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        إذا (dump_گراف_Level() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

عام:
    ساكن std::unique_ptr<Cfg_گراف> apply(Ast_ڳنڍ* stmtsp) {
        أرجع std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<Cfg_گراف> Cfg_گراف::build(Ast_ڳنڍ* stmtsp) { أرجع CfgBuilder::apply(stmtsp); }
