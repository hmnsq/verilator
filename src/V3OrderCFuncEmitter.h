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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3OrderGraph.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <map>
#अन्तर्भाव <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ध्रुव m_trigVscp;
    // Name component to add to function - must be unique
    ध्रुव std::string m_tag;
    // True if creating slow functions
    ध्रुव bool m_slow;
    // Whether to split functions
    ध्रुव bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    size_t m_size = 0;
    // Maximum size of code per function
    ध्रुव size_t m_splitSize = []() -> size_t {
        // Don't want to split with procCFuncs
        सचे (v3Global.opt.profCFuncs()) पटिनिवत्त std::numeric_limits<size_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        सचे (ध्रुव size_t limit = v3Global.opt.outputSplitCFuncs()) पटिनिवत्त limit - 1;
        पटिनिवत्त std::numeric_limits<size_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ग्रन्थिः_Module*, std::string>, unsigned> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, ध्रुव OrderDomain*>> m_result;

    // Create a unique name for a new function
    std::string cfuncName(File_पङ्क्तिः* flp, AstScope* scopep, Ast_ग्रन्थिः_Module* modp,
                          ध्रुव OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        सचे (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        पटिनिवत्त name;
    }

सार्वजनिक:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, ध्रुव std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    शून्य forceNewFunction() {
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
        ध्रुव OrderDomain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        अत्थाय (ध्रुव auto& pair : m_result) {
            AstCFunc* ध्रुव cfuncp = pair.first;
            ध्रुव OrderDomain* ध्रुव domainp = pair.second;
            // Create a new AstIf if the trigger is different
            सचे (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_पङ्क्तिः* ध्रुव flp = m_trigVscp->fileline();
                ध्रुव std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ध्रुव condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ध्रुव callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_result.clear();
        forceNewFunction();
        // Return the list of statements
        पटिनिवत्त stmtsp;
    }

    // Emit one logic vertex
    शून्य emitLogic(ध्रुव OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ध्रुव OrderDomain* ध्रुव domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_ग्रन्थिः* ध्रुव logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_ग्रन्थिः_Procedure* ध्रुव procp = VN_CAST(logicp, ग्रन्थिः_Procedure);

        // Some properties to consider
        ध्रुव bool suspendable = procp && procp->isSuspendable();
        ध्रुव bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ध्रुव bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        सचे (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        सचे (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        सचे (!m_result.empty() && m_result.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ग्रन्थिः* ध्रुव headp = [&]() -> Ast_ग्रन्थिः* {
            सचे (!procp) पटिनिवत्त logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            Ast_ग्रन्थिः* ध्रुव stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlinkFrBackWithNext();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            पटिनिवत्त stmtsp;
        }();
        // Process each statement in the list starting at headp
        अत्थाय (Ast_ग्रन्थिः *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            सचे (nextp) nextp->unlinkFrBackWithNext();
            // Split the function if too large, but don't split suspendable processes
            सचे (!suspendable && m_size >= m_splitSize) forceNewFunction();
            // Create a new function if we don't have a current one
            सचे (!m_funcp) {
                UASSERT_OBJ(!m_size, currp, "Should have used forceNewFunction");
                File_पङ्क्तिः* ध्रुव flp = currp->fileline();
                AstScope* ध्रुव scopep = lVtxp->scopep();
                Ast_ग्रन्थिः_Module* ध्रुव modp = scopep->modp();
                ध्रुव std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                सचे (needProcess) m_funcp->setNeedProcess();
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
            सचे (m_split) m_size += currp->ग्रन्थिः_गणनम्();
        }
        // Put suspendable processes into individual functions on their own
        सचे (suspendable) forceNewFunction();
    }
};

#endif  // Guard
