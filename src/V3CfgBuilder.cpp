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
    std::unique_ptr<Cfg_ګراف> m_cfgp{جديد Cfg_ګراف};
    // Current basic block to add statements to
    Cfg_بلاک* m_currBBp = تش_اشاره;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_بلاک*, Cfg_بلاک*> m_jump_بلاک_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_بلاک*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    فراغ جمع_کړه_Stmt(Ast_غوټه_Stmt* nodep) { m_currBBp->m_stmtps.emplace_شا(nodep); }

    // Used to handle statements not representable in the CFG
    فراغ nonRepresentable(Ast_غوټه_Stmt*) {
        إذا (!m_cfgp) أرجع;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    فراغ simpleStatement(Ast_غوټه_Stmt* nodep, منطقي representable = ریښتیا) {
        إذا (!m_cfgp) أرجع;
        // If non-representable, reset graph
        إذا (!representable) {
            m_cfgp.reset();
            أرجع;
        }
        // Just add to current block
        جمع_کړه_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (!m_cfgp) أرجع;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override { nonRepresentable(nodep); }
    فراغ لیدنه(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    فراغ لیدنه(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    فراغ لیدنه(Ast_ټاکل* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    فراغ لیدنه(AstComment*) override {}  // ignore entirely
    فراغ لیدنه(AstDisplay* nodep) override { simpleStatement(nodep); }
    فراغ لیدنه(Ast_پای* nodep) override { simpleStatement(nodep); }
    فراغ لیدنه(Ast_پای_Fork* nodep) override { simpleStatement(nodep); }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override { simpleStatement(nodep); }
    فراغ لیدنه(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    فراغ لیدنه(AstIf* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Add terminator statement to current block - semantically the condition check only ...
        جمع_کړه_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_بلاک* ثابت thenBBp = m_cfgp->جمع_کړه_بلاک();
        Cfg_بلاک* ثابت elseBBp = m_cfgp->جمع_کړه_بلاک();
        Cfg_بلاک* ثابت contBBp = m_cfgp->جمع_کړه_بلاک();
        m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, thenBBp);
        m_cfgp->جمع_کړه_Untkn_څنډه(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_بل_Const_Null(nodep->thensp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_بل_Const_Null(nodep->elsesp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ لیدنه(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        إذا (!m_cfgp) أرجع;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_بلاک* ثابت contBBp = m_cfgp->جمع_کړه_بلاک();
        ثابت منطقي new_ثبت = m_loopContp.emplace(nodep, contBBp).دوهم;
        UASSERT_OBJ(new_ثبت, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_بلاک* ثابت bodyBBp = m_cfgp->جمع_کړه_بلاک();
        m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_بل_Const_Null(nodep->stmtsp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ لیدنه(AstLoopTest* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Add terminator statement to current block - semantically the condition check only ...
        جمع_کړه_Stmt(nodep);

        // Create continuation blocks
        Cfg_بلاک* ثابت contBBp = m_cfgp->جمع_کړه_بلاک();
        m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, contBBp);
        m_cfgp->جمع_کړه_Untkn_څنډه(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_بلاک* ثابت contBBp = m_cfgp->جمع_کړه_بلاک();
        ثابت منطقي new_ثبت = m_jump_بلاک_Contp.emplace(nodep, contBBp).دوهم;
        UASSERT_OBJ(new_ثبت, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_بل_Const_Null(nodep->stmtsp());
        إذا (!m_cfgp) أرجع;
        إذا (m_currBBp) m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    فراغ لیدنه(AstJumpGo* nodep) override {
        إذا (!m_cfgp) أرجع;

        // Non-representable if not last in statement list (V3Const will fix this later)
        إذا (nodep->nextp()) {
            m_cfgp.reset();
            أرجع;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->جمع_کړه_اخیستل_شوی_څنډه(m_currBBp, m_jump_بلاک_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = تش_اشاره;
    }

    // CONSTRUCTOR
    څرګند CfgBuilder(Ast_غوټه* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->جمع_کړه_بلاک();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_بل_Const_Null(stmtsp);
        // If failed, stop now
        إذا (!m_cfgp) أرجع;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::ویکټور<V3_ګراف_راس*> unreachableps;
            لكل (V3_ګراف_راس* ثابت vtxp : m_cfgp->vertices().unlinkable()) {
                إذا (vtxp == m_cfgp->m_enterp) استمر;
                إذا (vtxp == m_cfgp->m_exitp) استمر;
                UASSERT_OBJ(!vtxp->بهر_تشه(), vtxp, "Block with no successor other than exit");
                إذا (vtxp->دننه_تشه()) unreachableps.emplace_شا(vtxp);
            }
            طالما (!unreachableps.تشه()) {
                V3_ګراف_راس* ثابت vtxp = unreachableps.شا();
                unreachableps.راوباسه_شا();
                لكل (ثابت V3_ګراف_څنډه& څنډه : vtxp->بهر_څنډې()) {
                    --m_cfgp->m_n_څنډې;
                    إذا (څنډه.سر()->دننه_Size1()) unreachableps.emplace_شا(څنډه.سر());
                }
                --m_cfgp->m_n_بلاکونه;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        إذا (dump_ګراف_Level() >= 9) {
            m_cfgp->rpo_بلاکونه();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        إذا (dump_ګراف_Level() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

عام:
    ساكن std::unique_ptr<Cfg_ګراف> apply(Ast_غوټه* stmtsp) {
        أرجع std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<Cfg_ګراف> Cfg_ګراف::build(Ast_غوټه* stmtsp) { أرجع CfgBuilder::apply(stmtsp); }
