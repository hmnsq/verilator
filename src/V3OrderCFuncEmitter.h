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

#ifndef VERILATOR_V3ORDERCFUNCEMITTER_H_
#define VERILATOR_V3ORDERCFUNCEMITTER_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ಧ್ರುವ m_trigVscp;
    // Name component to add to function - must be unique
    ಧ್ರುವ std::string m_tag;
    // True if creating slow functions
    ಧ್ರುವ bool m_slow;
    // Whether to split functions
    ಧ್ರುವ bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    size_t m_size = 0;
    // Maximum size of code per function
    ಧ್ರುವ size_t m_splitSize = []() -> size_t {
        // Don't want to split with procCFuncs
        ಯದಿ (v3Global.opt.profCFuncs()) ಪ್ರತ್ಯಾಗಮ std::numeric_limits<size_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        ಯದಿ (ಧ್ರುವ size_t limit = v3Global.opt.outputSplitCFuncs()) ಪ್ರತ್ಯಾಗಮ limit - 1;
        ಪ್ರತ್ಯಾಗಮ std::numeric_limits<size_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ಗಂಟು_Module*, std::string>, unsigned> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, ಧ್ರುವ OrderDomain*>> m_result;

    // Create a unique name for a new function
    std::string cfuncName(File_ಸಾಲು* flp, AstScope* scopep, Ast_ಗಂಟು_Module* modp,
                          ಧ್ರುವ OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        ಯದಿ (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        ಪ್ರತ್ಯಾಗಮ name;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, ಧ್ರುವ std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    ಶೂನ್ಯ forceNewFunction() {
        m_size = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        ಧ್ರುವ OrderDomain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_result) {
            AstCFunc* ಧ್ರುವ cfuncp = pair.first;
            ಧ್ರುವ OrderDomain* ಧ್ರುವ domainp = pair.second;
            // Create a new AstIf if the trigger is different
            ಯದಿ (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_ಸಾಲು* ಧ್ರುವ flp = m_trigVscp->fileline();
                ಧ್ರುವ std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ಧ್ರುವ condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ಗಂಟು::addNext(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ಧ್ರುವ callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_result.clear();
        forceNewFunction();
        // Return the list of statements
        ಪ್ರತ್ಯಾಗಮ stmtsp;
    }

    // Emit one logic vertex
    ಶೂನ್ಯ emitLogic(ಧ್ರುವ OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ಧ್ರುವ OrderDomain* ಧ್ರುವ domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_ಗಂಟು* ಧ್ರುವ logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_ಗಂಟು_Procedure* ಧ್ರುವ procp = VN_CAST(logicp, ಗಂಟು_Procedure);

        // Some properties to consider
        ಧ್ರುವ bool suspendable = procp && procp->isSuspendable();
        ಧ್ರುವ bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ಧ್ರುವ bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        ಯದಿ (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        ಯದಿ (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        ಯದಿ (!m_result.empty() && m_result.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ಗಂಟು* ಧ್ರುವ headp = [&]() -> Ast_ಗಂಟು* {
            ಯದಿ (!procp) ಪ್ರತ್ಯಾಗಮ logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlinkFrBackWithNext();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            ಪ್ರತ್ಯಾಗಮ stmtsp;
        }();
        // Process each statement in the list starting at headp
        ಪ್ರತಿ (Ast_ಗಂಟು *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            ಯದಿ (nextp) nextp->unlinkFrBackWithNext();
            // Split the function if too large, but don't split suspendable processes
            ಯದಿ (!suspendable && m_size >= m_splitSize) forceNewFunction();
            // Create a new function if we don't have a current one
            ಯದಿ (!m_funcp) {
                UASSERT_OBJ(!m_size, currp, "Should have used forceNewFunction");
                File_ಸಾಲು* ಧ್ರುವ flp = currp->fileline();
                AstScope* ಧ್ರುವ scopep = lVtxp->scopep();
                Ast_ಗಂಟು_Module* ಧ್ರುವ modp = scopep->modp();
                ಧ್ರುವ std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                ಯದಿ (needProcess) m_funcp->setNeedProcess();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->addBlocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_result.emplace_back(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->addStmtsp(currp);
            // If splitting, add in the size of the code we just added
            ಯದಿ (m_split) m_size += currp->ಗಂಟು_ಎಣಿಕೆ();
        }
        // Put suspendable processes into individual functions on their own
        ಯದಿ (suspendable) forceNewFunction();
    }
};

#endif  // Guard
