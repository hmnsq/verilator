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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3OrderGraph.h"
#অন্তর্ভাব "V3Sched.h"

#অন্তর্ভাব <limits>
#অন্তর্ভাব <map>
#অন্তর্ভাব <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ধ্রুব m_trigVscp;
    // Name component to add to function - must be unique
    ধ্রুব std::string m_tag;
    // True if creating slow functions
    ধ্রুব bool m_slow;
    // Whether to split functions
    ধ্রুব bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    size_t m_size = 0;
    // Maximum size of code per function
    ধ্রুব size_t m_splitSize = []() -> size_t {
        // Don't want to split with procCFuncs
        যদি (v3Global.opt.profCFuncs()) প্রত্যাগম std::numeric_limits<size_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        যদি (ধ্রুব size_t limit = v3Global.opt.outputSplitCFuncs()) প্রত্যাগম limit - 1;
        প্রত্যাগম std::numeric_limits<size_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_গ্রন্থি_Module*, std::string>, unsigned> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, ধ্রুব OrderDomain*>> m_result;

    // Create a unique name for a new function
    std::string cfuncName(File_সারি* flp, AstScope* scopep, Ast_গ্রন্থি_Module* modp,
                          ধ্রুব OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        যদি (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        প্রত্যাগম name;
    }

সার্বজনিক:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, ধ্রুব std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    শূন্য forceNewFunction() {
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
        ধ্রুব OrderDomain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        প্রতি (ধ্রুব auto& pair : m_result) {
            AstCFunc* ধ্রুব cfuncp = pair.first;
            ধ্রুব OrderDomain* ধ্রুব domainp = pair.second;
            // Create a new AstIf if the trigger is different
            যদি (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_সারি* ধ্রুব flp = m_trigVscp->fileline();
                ধ্রুব std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ধ্রুব condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_গ্রন্থি::addNext(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ধ্রুব callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_result.clear();
        forceNewFunction();
        // Return the list of statements
        প্রত্যাগম stmtsp;
    }

    // Emit one logic vertex
    শূন্য emitLogic(ধ্রুব OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ধ্রুব OrderDomain* ধ্রুব domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_গ্রন্থি* ধ্রুব logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_গ্রন্থি_Procedure* ধ্রুব procp = VN_CAST(logicp, গ্রন্থি_Procedure);

        // Some properties to consider
        ধ্রুব bool suspendable = procp && procp->isSuspendable();
        ধ্রুব bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ধ্রুব bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        যদি (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        যদি (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        যদি (!m_result.empty() && m_result.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_গ্রন্থি* ধ্রুব headp = [&]() -> Ast_গ্রন্থি* {
            যদি (!procp) প্রত্যাগম logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            Ast_গ্রন্থি* ধ্রুব stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlinkFrBackWithNext();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            প্রত্যাগম stmtsp;
        }();
        // Process each statement in the list starting at headp
        প্রতি (Ast_গ্রন্থি *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            যদি (nextp) nextp->unlinkFrBackWithNext();
            // Split the function if too large, but don't split suspendable processes
            যদি (!suspendable && m_size >= m_splitSize) forceNewFunction();
            // Create a new function if we don't have a current one
            যদি (!m_funcp) {
                UASSERT_OBJ(!m_size, currp, "Should have used forceNewFunction");
                File_সারি* ধ্রুব flp = currp->fileline();
                AstScope* ধ্রুব scopep = lVtxp->scopep();
                Ast_গ্রন্থি_Module* ধ্রুব modp = scopep->modp();
                ধ্রুব std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                যদি (needProcess) m_funcp->setNeedProcess();
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
            যদি (m_split) m_size += currp->গ্রন্থি_গুনতি();
        }
        // Put suspendable processes into individual functions on their own
        যদি (suspendable) forceNewFunction();
    }
};

#endif  // Guard
