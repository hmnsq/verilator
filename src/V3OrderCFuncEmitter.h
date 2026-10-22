// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Block code ordering
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
//  Class used to construct AstCFuncs from a sequence of OrderLogicVertex's
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3ORDERCFUNCEMITTER_H_
#నిర్వచించు VERILATOR_V3ORDERCFUNCEMITTER_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3OrderGraph.h"
#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ <limits>
#అన్తర్భావ <map>
#అన్తర్భావ <vector>

class V3_క్రమం_CFunc_Emitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* స్థిరరాశి m_trigVscp;
    // Name component to add to function - must be unique
    స్థిరరాశి std::string m_ట్యాగ్;
    // True if creating slow functions
    స్థిరరాశి bool m_slow;
    // Whether to split functions
    స్థిరరాశి bool m_విభజించు = v3Global.opt.output_విభజించు_CFuncs();
    // Size of code emitted so in the current function - for splitting
    కొలత_t m_కొలత = 0;
    // Maximum size of code per function
    స్థిరరాశి కొలత_t m_విభజించు_కొలత = []() -> కొలత_t {
        // Don't want to split with procCFuncs
        ఒకవేళ (v3Global.opt.profCFuncs()) తిరిగి std::numeric_limits<కొలత_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        ఒకవేళ (స్థిరరాశి కొలత_t హద్దు = v3Global.opt.output_విభజించు_CFuncs()) తిరిగి హద్దు - 1;
        తిరిగి std::numeric_limits<కొలత_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_కణుపు_Module*, std::string>, గుర్తులేని> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::సదిశ<std::pair<AstCFunc*, స్థిరరాశి క్రమం_Domain*>> m_ఫలితం;

    // Create a unique name for a new function
    std::string cfuncName(File_పంక్తి* flp, AstScope* scopep, Ast_కణుపు_Module* modp,
                          స్థిరరాశి క్రమం_Logic_శీర్షం* lVtxp) {
        std::string name = "_" + m_ట్యాగ్;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::గమ్యం_string(m_funcNums[{modp, name}]++);
        ఒకవేళ (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        తిరిగి name;
    }

బహిరంగ:
    // CONSTRUCTOR
    V3_క్రమం_CFunc_Emitter(AstVarScope* trigVscp, స్థిరరాశి std::string& ట్యాగ్, bool slow)
        : m_trigVscp{trigVscp}
        , m_ట్యాగ్{ట్యాగ్}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3_క్రమం_CFunc_Emitter);
    VL_UNMOVABLE(V3_క్రమం_CFunc_Emitter);

    // Force the creation of a new function
    శూన్య forceNewFunction() {
        m_కొలత = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        స్థిరరాశి క్రమం_Domain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_ఫలితం) {
            AstCFunc* స్థిరరాశి cfuncp = pair.మొదటి;
            స్థిరరాశి క్రమం_Domain* స్థిరరాశి domainp = pair.రెండవ;
            // Create a new AstIf if the trigger is different
            ఒకవేళ (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_పంక్తి* స్థిరరాశి flp = m_trigVscp->fileline();
                స్థిరరాశి std::సమితి<uint32_t>& triggers = domainp->triggers();
                AstTriggered* స్థిరరాశి condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.చివర()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* స్థిరరాశి callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtype_సమితి_Void();
            currp->కూడు_Thensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_ఫలితం.clear();
        forceNewFunction();
        // Return the list of statements
        తిరిగి stmtsp;
    }

    // Emit one logic vertex
    శూన్య emitLogic(స్థిరరాశి క్రమం_Logic_శీర్షం* lVtxp) {
        // Sensitivity domain of logic we are emitting
        స్థిరరాశి క్రమం_Domain* స్థిరరాశి domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_కణుపు* స్థిరరాశి logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_కణుపు_Procedure* స్థిరరాశి procp = VN_CAST(logicp, కణుపు_Procedure);

        // Some properties to consider
        స్థిరరాశి bool suspendable = procp && procp->isSuspendable();
        స్థిరరాశి bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        స్థిరరాశి bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        ఒకవేళ (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        ఒకవేళ (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        ఒకవేళ (!m_ఫలితం.ఖాళీలు() && m_ఫలితం.వెనుక().రెండవ != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_కణుపు* స్థిరరాశి headp = [&]() -> Ast_కణుపు* {
            ఒకవేళ (!procp) తిరిగి logicp->unlink_Fr_వెనుక();  // Not a procedure, handle as a unit
            Ast_కణుపు* స్థిరరాశి stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            తిరిగి stmtsp;
        }();
        // Process each statement in the list starting at headp
        కొరకు (Ast_కణుపు *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            ఒకవేళ (nextp) nextp->unlink_Fr_వెనుక_With_తదుపరి();
            // Split the function if too large, but don't split suspendable processes
            ఒకవేళ (!suspendable && m_కొలత >= m_విభజించు_కొలత) forceNewFunction();
            // Create a new function if we don't have a current one
            ఒకవేళ (!m_funcp) {
                UASSERT_OBJ(!m_కొలత, currp, "Should have used forceNewFunction");
                File_పంక్తి* స్థిరరాశి flp = currp->fileline();
                AstScope* స్థిరరాశి scopep = lVtxp->scopep();
                Ast_కణుపు_Module* స్థిరరాశి modp = scopep->modp();
                స్థిరరాశి std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                ఒకవేళ (needProcess) m_funcp->సమితి_Need_Process();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->కూడు_Blocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_ఫలితం.emplace_వెనుక(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->కూడు_Stmtsp(currp);
            // If splitting, add in the size of the code we just added
            ఒకవేళ (m_విభజించు) m_కొలత += currp->కణుపు_లెక్క();
        }
        // Put suspendable processes into individual functions on their own
        ఒకవేళ (suspendable) forceNewFunction();
    }
};

#అయితే_ముగింపు  // Guard
