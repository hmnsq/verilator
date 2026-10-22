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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3OrderGraph.h"
#അന്തര്ഭാവ "V3Sched.h"

#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* സ്ഥിരം m_trigVscp;
    // Name component to add to function - must be unique
    സ്ഥിരം std::string m_tag;
    // True if creating slow functions
    സ്ഥിരം bool m_slow;
    // Whether to split functions
    സ്ഥിരം bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    size_t m_size = 0;
    // Maximum size of code per function
    സ്ഥിരം size_t m_splitSize = []() -> size_t {
        // Don't want to split with procCFuncs
        എങ്കിൽ (v3Global.opt.profCFuncs()) തിരികെ std::numeric_limits<size_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        എങ്കിൽ (സ്ഥിരം size_t limit = v3Global.opt.outputSplitCFuncs()) തിരികെ limit - 1;
        തിരികെ std::numeric_limits<size_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_കണ്ണി_Module*, std::string>, unsigned> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, സ്ഥിരം OrderDomain*>> m_result;

    // Create a unique name for a new function
    std::string cfuncName(File_വരി* flp, AstScope* scopep, Ast_കണ്ണി_Module* modp,
                          സ്ഥിരം OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        എങ്കിൽ (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        തിരികെ name;
    }

പൊതു:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, സ്ഥിരം std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    ശൂന്യം forceNewFunction() {
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
        സ്ഥിരം OrderDomain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        വേണ്ടി (സ്ഥിരം auto& pair : m_result) {
            AstCFunc* സ്ഥിരം cfuncp = pair.first;
            സ്ഥിരം OrderDomain* സ്ഥിരം domainp = pair.second;
            // Create a new AstIf if the trigger is different
            എങ്കിൽ (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_വരി* സ്ഥിരം flp = m_trigVscp->fileline();
                സ്ഥിരം std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* സ്ഥിരം condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_കണ്ണി::addNext(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* സ്ഥിരം callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_result.clear();
        forceNewFunction();
        // Return the list of statements
        തിരികെ stmtsp;
    }

    // Emit one logic vertex
    ശൂന്യം emitLogic(സ്ഥിരം OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        സ്ഥിരം OrderDomain* സ്ഥിരം domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_കണ്ണി* സ്ഥിരം logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_കണ്ണി_Procedure* സ്ഥിരം procp = VN_CAST(logicp, കണ്ണി_Procedure);

        // Some properties to consider
        സ്ഥിരം bool suspendable = procp && procp->isSuspendable();
        സ്ഥിരം bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        സ്ഥിരം bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        എങ്കിൽ (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        എങ്കിൽ (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        എങ്കിൽ (!m_result.empty() && m_result.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_കണ്ണി* സ്ഥിരം headp = [&]() -> Ast_കണ്ണി* {
            എങ്കിൽ (!procp) തിരികെ logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            Ast_കണ്ണി* സ്ഥിരം stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlinkFrBackWithNext();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            തിരികെ stmtsp;
        }();
        // Process each statement in the list starting at headp
        വേണ്ടി (Ast_കണ്ണി *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            എങ്കിൽ (nextp) nextp->unlinkFrBackWithNext();
            // Split the function if too large, but don't split suspendable processes
            എങ്കിൽ (!suspendable && m_size >= m_splitSize) forceNewFunction();
            // Create a new function if we don't have a current one
            എങ്കിൽ (!m_funcp) {
                UASSERT_OBJ(!m_size, currp, "Should have used forceNewFunction");
                File_വരി* സ്ഥിരം flp = currp->fileline();
                AstScope* സ്ഥിരം scopep = lVtxp->scopep();
                Ast_കണ്ണി_Module* സ്ഥിരം modp = scopep->modp();
                സ്ഥിരം std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                എങ്കിൽ (needProcess) m_funcp->setNeedProcess();
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
            എങ്കിൽ (m_split) m_size += currp->കണ്ണി_എണ്ണം();
        }
        // Put suspendable processes into individual functions on their own
        എങ്കിൽ (suspendable) forceNewFunction();
    }
};

#endif  // Guard
