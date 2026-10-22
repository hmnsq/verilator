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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Cfg.h"
#شامل "V3EmitV.h"

#شامل <deque>
#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت CfgBuilder final : عوامی VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<Cfg_گراف> m_cfgp{نیا Cfg_گراف};
    // Current basic block to add statements to
    Cfg_بلاک* m_currBBp = خالی_اشاریہ;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_بلاک*, Cfg_بلاک*> m_jump_بلاک_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_بلاک*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    خالی جوڑو_Stmt(Ast_گرہ_Stmt* nodep) { m_currBBp->m_stmtps.emplace_عقب(nodep); }

    // Used to handle statements not representable in the CFG
    خالی nonRepresentable(Ast_گرہ_Stmt*) {
        اگر (!m_cfgp) واپس;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    خالی simpleStatement(Ast_گرہ_Stmt* nodep, منطقی representable = سچ) {
        اگر (!m_cfgp) واپس;
        // If non-representable, reset graph
        اگر (!representable) {
            m_cfgp.reset();
            واپس;
        }
        // Just add to current block
        جوڑو_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (!m_cfgp) واپس;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    خالی دورہ(Ast_تفویض_Dly* nodep) override { nonRepresentable(nodep); }
    خالی دورہ(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    خالی دورہ(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    خالی دورہ(Ast_تفویض* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    خالی دورہ(Ast_تفویض_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    خالی دورہ(AstComment*) override {}  // ignore entirely
    خالی دورہ(AstDisplay* nodep) override { simpleStatement(nodep); }
    خالی دورہ(Ast_اختتام* nodep) override { simpleStatement(nodep); }
    خالی دورہ(Ast_اختتام_Fork* nodep) override { simpleStatement(nodep); }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override { simpleStatement(nodep); }
    خالی دورہ(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    خالی دورہ(AstIf* nodep) override {
        اگر (!m_cfgp) واپس;

        // Add terminator statement to current block - semantically the condition check only ...
        جوڑو_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_بلاک* ثابت thenBBp = m_cfgp->جوڑو_بلاک();
        Cfg_بلاک* ثابت elseBBp = m_cfgp->جوڑو_بلاک();
        Cfg_بلاک* ثابت contBBp = m_cfgp->جوڑو_بلاک();
        m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, thenBBp);
        m_cfgp->جوڑو_Untkn_کنارہ(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_اگلا_Const_Null(nodep->thensp());
        اگر (!m_cfgp) واپس;
        اگر (m_currBBp) m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_اگلا_Const_Null(nodep->elsesp());
        اگر (!m_cfgp) واپس;
        اگر (m_currBBp) m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    خالی دورہ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        اگر (!m_cfgp) واپس;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_بلاک* ثابت contBBp = m_cfgp->جوڑو_بلاک();
        ثابت منطقی new_اندراج = m_loopContp.emplace(nodep, contBBp).دوسرا;
        UASSERT_OBJ(new_اندراج, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_بلاک* ثابت bodyBBp = m_cfgp->جوڑو_بلاک();
        m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        اگر (!m_cfgp) واپس;
        اگر (m_currBBp) m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    خالی دورہ(AstLoopTest* nodep) override {
        اگر (!m_cfgp) واپس;

        // Add terminator statement to current block - semantically the condition check only ...
        جوڑو_Stmt(nodep);

        // Create continuation blocks
        Cfg_بلاک* ثابت contBBp = m_cfgp->جوڑو_بلاک();
        m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, contBBp);
        m_cfgp->جوڑو_Untkn_کنارہ(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    خالی دورہ(Ast_Jump_بلاک* nodep) override {
        اگر (!m_cfgp) واپس;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_بلاک* ثابت contBBp = m_cfgp->جوڑو_بلاک();
        ثابت منطقی new_اندراج = m_jump_بلاک_Contp.emplace(nodep, contBBp).دوسرا;
        UASSERT_OBJ(new_اندراج, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        اگر (!m_cfgp) واپس;
        اگر (m_currBBp) m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    خالی دورہ(AstJumpGo* nodep) override {
        اگر (!m_cfgp) واپس;

        // Non-representable if not last in statement list (V3Const will fix this later)
        اگر (nodep->nextp()) {
            m_cfgp.reset();
            واپس;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->جوڑو_لیا_گیا_کنارہ(m_currBBp, m_jump_بلاک_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = خالی_اشاریہ;
    }

    // CONSTRUCTOR
    صریح CfgBuilder(Ast_گرہ* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->جوڑو_بلاک();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_اگلا_Const_Null(stmtsp);
        // If failed, stop now
        اگر (!m_cfgp) واپس;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::سمتیہ<V3_گراف_راس*> unreachableps;
            برائے (V3_گراف_راس* ثابت vtxp : m_cfgp->vertices().unlinkable()) {
                اگر (vtxp == m_cfgp->m_enterp) جاری;
                اگر (vtxp == m_cfgp->m_exitp) جاری;
                UASSERT_OBJ(!vtxp->برآمد_خالی_خانے(), vtxp, "Block with no successor other than exit");
                اگر (vtxp->اندر_خالی_خانے()) unreachableps.emplace_عقب(vtxp);
            }
            جب (!unreachableps.خالی_خانے()) {
                V3_گراف_راس* ثابت vtxp = unreachableps.عقب();
                unreachableps.نکالو_عقب();
                برائے (ثابت V3_گراف_کنارہ& کنارہ : vtxp->برآمد_کنارے()) {
                    --m_cfgp->m_n_کنارے;
                    اگر (کنارہ.چوٹی()->اندر_Size1()) unreachableps.emplace_عقب(کنارہ.چوٹی());
                }
                --m_cfgp->m_n_بلاکیں;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        اگر (dump_گراف_Level() >= 9) {
            m_cfgp->rpo_بلاکیں();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        اگر (dump_گراف_Level() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

عوامی:
    ساکن std::unique_ptr<Cfg_گراف> apply(Ast_گرہ* stmtsp) {
        واپس std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<Cfg_گراف> Cfg_گراف::build(Ast_گرہ* stmtsp) { واپس CfgBuilder::apply(stmtsp); }
