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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3OrderGraph.h"
#ضمن "V3Sched.h"

#ضمن <limits>
#ضمن <map>
#ضمن <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ثابت m_trigVscp;
    // Name component to add to function - must be unique
    ثابت std::string m_tag;
    // True if creating slow functions
    ثابت bool m_slow;
    // Whether to split functions
    ثابت bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    size_t m_size = 0;
    // Maximum size of code per function
    ثابت size_t m_splitSize = []() -> size_t {
        // Don't want to split with procCFuncs
        𐤀𐤌 (v3Global.opt.profCFuncs()) أرجع std::numeric_limits<size_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        𐤀𐤌 (ثابت size_t limit = v3Global.opt.outputSplitCFuncs()) أرجع limit - 1;
        أرجع std::numeric_limits<size_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<AstNodeModule*, std::string>, unsigned> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, ثابت OrderDomain*>> m_result;

    // Create a unique name for a new function
    std::string cfuncName(FileLine* flp, AstScope* scopep, AstNodeModule* modp,
                          ثابت OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        𐤀𐤌 (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        أرجع name;
    }

عام:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, ثابت std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    فراغ forceNewFunction() {
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
        ثابت OrderDomain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        لكل (ثابت auto& pair : m_result) {
            AstCFunc* ثابت cfuncp = pair.first;
            ثابت OrderDomain* ثابت domainp = pair.second;
            // Create a new AstIf if the trigger is different
            𐤀𐤌 (domainp != prevDomainp) {
                prevDomainp = domainp;
                FileLine* ثابت flp = m_trigVscp->fileline();
                ثابت std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ثابت condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = AstNode::addNext(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ثابت callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_result.clear();
        forceNewFunction();
        // Return the list of statements
        أرجع stmtsp;
    }

    // Emit one logic vertex
    فراغ emitLogic(ثابت OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ثابت OrderDomain* ثابت domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        AstNode* ثابت logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        AstNodeProcedure* ثابت procp = VN_CAST(logicp, NodeProcedure);

        // Some properties to consider
        ثابت bool suspendable = procp && procp->isSuspendable();
        ثابت bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ثابت bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        𐤀𐤌 (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        𐤀𐤌 (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        𐤀𐤌 (!m_result.empty() && m_result.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        AstNode* ثابت headp = [&]() -> AstNode* {
            𐤀𐤌 (!procp) أرجع logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            AstNode* ثابت stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlinkFrBackWithNext();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            أرجع stmtsp;
        }();
        // Process each statement in the list starting at headp
        لكل (AstNode *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            𐤀𐤌 (nextp) nextp->unlinkFrBackWithNext();
            // Split the function if too large, but don't split suspendable processes
            𐤀𐤌 (!suspendable && m_size >= m_splitSize) forceNewFunction();
            // Create a new function if we don't have a current one
            𐤀𐤌 (!m_funcp) {
                UASSERT_OBJ(!m_size, currp, "Should have used forceNewFunction");
                FileLine* ثابت flp = currp->fileline();
                AstScope* ثابت scopep = lVtxp->scopep();
                AstNodeModule* ثابت modp = scopep->modp();
                ثابت std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                𐤀𐤌 (needProcess) m_funcp->setNeedProcess();
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
            𐤀𐤌 (m_split) m_size += currp->nodeCount();
        }
        // Put suspendable processes into individual functions on their own
        𐤀𐤌 (suspendable) forceNewFunction();
    }
};

#endif  // Guard
