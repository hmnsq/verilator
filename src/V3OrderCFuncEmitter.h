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

#अगर_अघोषित VERILATOR_V3ORDERCFUNCEMITTER_H_
#घोषणा VERILATOR_V3ORDERCFUNCEMITTER_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Graph.h"
#समावेश "V3OrderGraph.h"
#समावेश "V3Sched.h"

#समावेश <limits>
#समावेश <map>
#समावेश <vector>

class V3OrderCFuncEmitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* स्थिर m_trigVscp;
    // Name component to add to function - must be unique
    स्थिर std::string m_tag;
    // True if creating slow functions
    स्थिर bool m_slow;
    // Whether to split functions
    स्थिर bool m_split = v3Global.opt.outputSplitCFuncs();
    // Size of code emitted so in the current function - for splitting
    माप_प्रकार m_आकार = 0;
    // Maximum size of code per function
    स्थिर माप_प्रकार m_split_आकार = []() -> माप_प्रकार {
        // Don't want to split with procCFuncs
        अगर (v3Global.opt.profCFuncs()) वापस std::numeric_limits<माप_प्रकार>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        अगर (स्थिर माप_प्रकार limit = v3Global.opt.outputSplitCFuncs()) वापस limit - 1;
        वापस std::numeric_limits<माप_प्रकार>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_गाँठ_Module*, std::string>, अचिन्हित> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::vector<std::pair<AstCFunc*, स्थिर OrderDomain*>> m_परिणाम;

    // Create a unique name for a new function
    std::string cfuncName(File_पंक्ति* flp, AstScope* scopep, Ast_गाँठ_Module* modp,
                          स्थिर OrderLogicVertex* lVtxp) {
        std::string name = "_" + m_tag;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::to_string(m_funcNums[{modp, name}]++);
        अगर (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        वापस name;
    }

खुला:
    // CONSTRUCTOR
    V3OrderCFuncEmitter(AstVarScope* trigVscp, स्थिर std::string& tag, bool slow)
        : m_trigVscp{trigVscp}
        , m_tag{tag}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3OrderCFuncEmitter);
    VL_UNMOVABLE(V3OrderCFuncEmitter);

    // Force the creation of a new function
    व्योम forceNewFunction() {
        m_आकार = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        स्थिर OrderDomain* पूर्व_Domainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        क्रम (स्थिर स्वतः& pair : m_परिणाम) {
            AstCFunc* स्थिर cfuncp = pair.first;
            स्थिर OrderDomain* स्थिर domainp = pair.second;
            // Create a new AstIf if the trigger is different
            अगर (domainp != पूर्व_Domainp) {
                पूर्व_Domainp = domainp;
                File_पंक्ति* स्थिर flp = m_trigVscp->fileline();
                स्थिर std::set<uint32_t>& triggers = domainp->triggers();
                AstTriggered* स्थिर condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.end()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_गाँठ::add_अग्र(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* स्थिर callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtypeSetVoid();
            currp->addThensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_परिणाम.clear();
        forceNewFunction();
        // Return the list of statements
        वापस stmtsp;
    }

    // Emit one logic vertex
    व्योम emitLogic(स्थिर OrderLogicVertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        स्थिर OrderDomain* स्थिर domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_गाँठ* स्थिर logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_गाँठ_Procedure* स्थिर procp = VN_CAST(logicp, गाँठ_Procedure);

        // Some properties to consider
        स्थिर bool suspendable = procp && procp->isSuspendable();
        स्थिर bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        स्थिर bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        अगर (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        अगर (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        अगर (!m_परिणाम.empty() && m_परिणाम.back().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_गाँठ* स्थिर headp = [&]() -> Ast_गाँठ* {
            अगर (!procp) वापस logicp->unlinkFrBack();  // Not a procedure, handle as a unit
            Ast_गाँठ* स्थिर stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_Back_With_अग्र();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            वापस stmtsp;
        }();
        // Process each statement in the list starting at headp
        क्रम (Ast_गाँठ *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            अगर (nextp) nextp->unlink_Fr_Back_With_अग्र();
            // Split the function if too large, but don't split suspendable processes
            अगर (!suspendable && m_आकार >= m_split_आकार) forceNewFunction();
            // Create a new function if we don't have a current one
            अगर (!m_funcp) {
                UASSERT_OBJ(!m_आकार, currp, "Should have used forceNewFunction");
                File_पंक्ति* स्थिर flp = currp->fileline();
                AstScope* स्थिर scopep = lVtxp->scopep();
                Ast_गाँठ_Module* स्थिर modp = scopep->modp();
                स्थिर std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                अगर (needProcess) m_funcp->setNeedProcess();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->addBlocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_परिणाम.emplace_back(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->addStmtsp(currp);
            // If splitting, add in the size of the code we just added
            अगर (m_split) m_आकार += currp->गाँठ_गिनती();
        }
        // Put suspendable processes into individual functions on their own
        अगर (suspendable) forceNewFunction();
    }
};

#पूर्ण_अगर  // Guard
