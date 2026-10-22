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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ଅଚଳ m_trigVscp;
    // Name component to add to function - must be unique
    ଅଚଳ std::string m_tag;
    // True if creating slow functions
    ଅଚଳ bool m_slow;
    // Whether to split functions
    ଅଚଳ bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    size_t m_size = 0;
    // Maximum size of code per function
    ଅଚଳ size_t m_splitSize = []() -> size_t {
        // Don't want to split with procCFuncs
        ଯଦି (v3Global.opt.profCFuncs()) ଫେରନ୍ତୁ std::numeric_limits<size_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        ଯଦି (ଅଚଳ size_t limit = v3Global.opt.outputSplitCFuncs()) ଫେରନ୍ତୁ limit - 1;
        ଫେରନ୍ତୁ std::numeric_limits<size_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ଗଣ୍ଠି_Module*, std::string>, unsigned> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, ଅଚଳ OrderDomain*>> m_result;

    // Create a unique name for a new function
    std::string cfuncName(File_ଧାଡ଼ି* flp, AstScope* scopep, Ast_ଗଣ୍ଠି_Module* modp,
                          ଅଚଳ OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        ଯଦି (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        ଫେରନ୍ତୁ name;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, ଅଚଳ std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    ଶୂନ୍ୟ forceNewFunction() {
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
        ଅଚଳ OrderDomain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        ପାଇଁ (ଅଚଳ auto& pair : m_result) {
            AstCFunc* ଅଚଳ cfuncp = pair.first;
            ଅଚଳ OrderDomain* ଅଚଳ domainp = pair.second;
            // Create a new AstIf if the trigger is different
            ଯଦି (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_ଧାଡ଼ି* ଅଚଳ flp = m_trigVscp->fileline();
                ଅଚଳ std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ଅଚଳ condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ଅଚଳ callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_result.clear();
        forceNewFunction();
        // Return the list of statements
        ଫେରନ୍ତୁ stmtsp;
    }

    // Emit one logic vertex
    ଶୂନ୍ୟ emitLogic(ଅଚଳ OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ଅଚଳ OrderDomain* ଅଚଳ domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_ଗଣ୍ଠି* ଅଚଳ logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ procp = VN_CAST(logicp, ଗଣ୍ଠି_Procedure);

        // Some properties to consider
        ଅଚଳ bool suspendable = procp && procp->isSuspendable();
        ଅଚଳ bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ଅଚଳ bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        ଯଦି (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        ଯଦି (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        ଯଦି (!m_result.empty() && m_result.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ଗଣ୍ଠି* ଅଚଳ headp = [&]() -> Ast_ଗଣ୍ଠି* {
            ଯଦି (!procp) ଫେରନ୍ତୁ logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlinkFrBackWithNext();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            ଫେରନ୍ତୁ stmtsp;
        }();
        // Process each statement in the list starting at headp
        ପାଇଁ (Ast_ଗଣ୍ଠି *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            ଯଦି (nextp) nextp->unlinkFrBackWithNext();
            // Split the function if too large, but don't split suspendable processes
            ଯଦି (!suspendable && m_size >= m_splitSize) forceNewFunction();
            // Create a new function if we don't have a current one
            ଯଦି (!m_funcp) {
                UASSERT_OBJ(!m_size, currp, "Should have used forceNewFunction");
                File_ଧାଡ଼ି* ଅଚଳ flp = currp->fileline();
                AstScope* ଅଚଳ scopep = lVtxp->scopep();
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = scopep->modp();
                ଅଚଳ std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                ଯଦି (needProcess) m_funcp->setNeedProcess();
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
            ଯଦି (m_split) m_size += currp->ଗଣ୍ଠି_ଗଣନା();
        }
        // Put suspendable processes into individual functions on their own
        ଯଦି (suspendable) forceNewFunction();
    }
};

#endif  // Guard
