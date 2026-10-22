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

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Cfg.h"
#అన్తర్భావ "V3EmitV.h"

#అన్తర్భావ <deque>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి CfgBuilder final : బహిరంగ VNVisitorConst {
    // STATE
    // The graph being built, or nullptr if failed to build one
    std::unique_ptr<Cfg_లేఖాచిత్రం> m_cfgp{కొత్త Cfg_లేఖాచిత్రం};
    // Current basic block to add statements to
    Cfg_ఖండం* m_currBBp = శూన్యసూచిక;
    // Continuation block for given JumpBlock
    std::unordered_map<Ast_Jump_ఖండం*, Cfg_ఖండం*> m_jump_ఖండం_Contp;
    // Continuation block for given Loop
    std::unordered_map<AstLoop*, Cfg_ఖండం*> m_loopContp;

    // METHODS

    // Add the given statement to the current CfgBlock
    శూన్య కూడు_Stmt(Ast_కణుపు_Stmt* nodep) { m_currBBp->m_stmtps.emplace_వెనుక(nodep); }

    // Used to handle statements not representable in the CFG
    శూన్య nonRepresentable(Ast_కణుపు_Stmt*) {
        ఒకవేళ (!m_cfgp) తిరిగి;
        m_cfgp.reset();
    }

    // Used to handle simple (non-branching) statements in the CFG
    శూన్య simpleStatement(Ast_కణుపు_Stmt* nodep, తార్కిక representable = నిజం) {
        ఒకవేళ (!m_cfgp) తిరిగి;
        // If non-representable, reset graph
        ఒకవేళ (!representable) {
            m_cfgp.reset();
            తిరిగి;
        }
        // Just add to current block
        కూడు_Stmt(nodep);
    }

    // VISITORS

    // Eventually we should handle all procedural statements, however, what
    // is a procedural statemen is a bit unclear (#6280), so in the first
    // instance we will only handle select statemetns that cover the requied
    // use cases, and in the base case we conservatively assume the statement
    // is non-representable. More visits can be added case by case if needed.
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (!m_cfgp) తిరిగి;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
        m_cfgp.reset();
    }

    // Non-representable statements
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override { nonRepresentable(nodep); }
    శూన్య సందర్శించు(AstCase* nodep) override { nonRepresentable(nodep); }  // V3Case will eliminate
    శూన్య సందర్శించు(AstDelay* nodep) override { nonRepresentable(nodep); }

    // Representable non control-flow statements
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override { simpleStatement(nodep, !nodep->timingControlp()); }
    శూన్య సందర్శించు(AstComment*) override {}  // ignore entirely
    శూన్య సందర్శించు(AstDisplay* nodep) override { simpleStatement(nodep); }
    శూన్య సందర్శించు(Ast_ముగింపు* nodep) override { simpleStatement(nodep); }
    శూన్య సందర్శించు(Ast_ముగింపు_Fork* nodep) override { simpleStatement(nodep); }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override { simpleStatement(nodep); }
    శూన్య సందర్శించు(AstStop* nodep) override { simpleStatement(nodep); }

    // Representable control flow statements
    శూన్య సందర్శించు(AstIf* nodep) override {
        ఒకవేళ (!m_cfgp) తిరిగి;

        // Add terminator statement to current block - semantically the condition check only ...
        కూడు_Stmt(nodep);

        // Create then/else/continuation blocks
        Cfg_ఖండం* స్థిరరాశి thenBBp = m_cfgp->కూడు_ఖండం();
        Cfg_ఖండం* స్థిరరాశి elseBBp = m_cfgp->కూడు_ఖండం();
        Cfg_ఖండం* స్థిరరాశి contBBp = m_cfgp->కూడు_ఖండం();
        m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, thenBBp);
        m_cfgp->కూడు_Untkn_అంచు(m_currBBp, elseBBp);

        // Build then branch
        m_currBBp = thenBBp;
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        ఒకవేళ (!m_cfgp) తిరిగి;
        ఒకవేళ (m_currBBp) m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, contBBp);

        // Build else branch
        m_currBBp = elseBBp;
        iterate_And_తదుపరి_Const_Null(nodep->elsesp());
        ఒకవేళ (!m_cfgp) తిరిగి;
        ఒకవేళ (m_currBBp) m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ఒకవేళ (!m_cfgp) తిరిగి;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_ఖండం* స్థిరరాశి contBBp = m_cfgp->కూడు_ఖండం();
        స్థిరరాశి తార్కిక new_నమోదు = m_loopContp.emplace(nodep, contBBp).రెండవ;
        UASSERT_OBJ(new_నమోదు, nodep, "AstLoop visited twice");

        // Create the body block
        Cfg_ఖండం* స్థిరరాశి bodyBBp = m_cfgp->కూడు_ఖండం();
        m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, bodyBBp);

        // Build the body
        m_currBBp = bodyBBp;
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        ఒకవేళ (!m_cfgp) తిరిగి;
        ఒకవేళ (m_currBBp) m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, bodyBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        ఒకవేళ (!m_cfgp) తిరిగి;

        // Add terminator statement to current block - semantically the condition check only ...
        కూడు_Stmt(nodep);

        // Create continuation blocks
        Cfg_ఖండం* స్థిరరాశి contBBp = m_cfgp->కూడు_ఖండం();
        m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, contBBp);
        m_cfgp->కూడు_Untkn_అంచు(m_currBBp, m_loopContp.at(nodep->loopp()));

        // Set continuation
        m_currBBp = contBBp;
    }
    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        ఒకవేళ (!m_cfgp) తిరిగి;

        // Don't acutally need to add this 'nodep' to any block

        // Create continuation block
        Cfg_ఖండం* స్థిరరాశి contBBp = m_cfgp->కూడు_ఖండం();
        స్థిరరాశి తార్కిక new_నమోదు = m_jump_ఖండం_Contp.emplace(nodep, contBBp).రెండవ;
        UASSERT_OBJ(new_నమోదు, nodep, "AstJumpBlock visited twice");

        // Build the body
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        ఒకవేళ (!m_cfgp) తిరిగి;
        ఒకవేళ (m_currBBp) m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, contBBp);

        // Set continuation
        m_currBBp = contBBp;
    }
    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        ఒకవేళ (!m_cfgp) తిరిగి;

        // Non-representable if not last in statement list (V3Const will fix this later)
        ఒకవేళ (nodep->nextp()) {
            m_cfgp.reset();
            తిరిగి;
        }

        // Don't acutally need to add this 'nodep' to any block

        // Make current block go to the continuation of the JumpBlock
        m_cfgp->కూడు_తీసుకున్న_అంచు(m_currBBp, m_jump_ఖండం_Contp.at(nodep->blockp()));

        // There should be no statements after a JumpGo!
        m_currBBp = శూన్యసూచిక;
    }

    // CONSTRUCTOR
    స్పష్టమైన CfgBuilder(Ast_కణుపు* stmtsp) {
        // Build the graph, starting from the entry block
        m_currBBp = m_cfgp->కూడు_ఖండం();
        m_cfgp->m_enterp = m_currBBp;
        // Visit each statement to build the control flow graph
        iterate_And_తదుపరి_Const_Null(stmtsp);
        // If failed, stop now
        ఒకవేళ (!m_cfgp) తిరిగి;
        // The final block is the exit block
        m_cfgp->m_exitp = m_currBBp;
        // Some blocks might not have predecessors if they are unreachable, remove them
        {
            std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*> unreachableps;
            కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_cfgp->vertices().unlinkable()) {
                ఒకవేళ (vtxp == m_cfgp->m_enterp) కొనసాగు;
                ఒకవేళ (vtxp == m_cfgp->m_exitp) కొనసాగు;
                UASSERT_OBJ(!vtxp->బహిర్గతం_ఖాళీలు(), vtxp, "Block with no successor other than exit");
                ఒకవేళ (vtxp->లోపల_ఖాళీలు()) unreachableps.emplace_వెనుక(vtxp);
            }
            అయితే (!unreachableps.ఖాళీలు()) {
                V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp = unreachableps.వెనుక();
                unreachableps.తీయి_వెనుక();
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->బహిర్గతం_అంచులు()) {
                    --m_cfgp->m_n_అంచులు;
                    ఒకవేళ (అంచు.శిఖరం()->లోపల_Size1()) unreachableps.emplace_వెనుక(అంచు.శిఖరం());
                }
                --m_cfgp->m_n_ఖండాలు;
                VL_DO_DANGLING(vtxp->unlinkDelete(m_cfgp.get()), vtxp);
            }
        }
        // Dump the initial graph
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) {
            m_cfgp->rpo_ఖండాలు();
            m_cfgp->dumpDotFilePrefixed("cfg-builder-initial");
        }
        // Minimize it
        m_cfgp->minimize();
        // Dump the final graph
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 8) m_cfgp->dumpDotFilePrefixed("cfg-builder");
    }

బహిరంగ:
    స్థిర std::unique_ptr<Cfg_లేఖాచిత్రం> apply(Ast_కణుపు* stmtsp) {
        తిరిగి std::move(CfgBuilder{stmtsp}.m_cfgp);
    }
};

std::unique_ptr<Cfg_లేఖాచిత్రం> Cfg_లేఖాచిత్రం::build(Ast_కణుపు* stmtsp) { తిరిగి CfgBuilder::apply(stmtsp); }
