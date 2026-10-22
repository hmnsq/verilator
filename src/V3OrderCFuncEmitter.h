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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ORDERCFUNCEMITTER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ORDERCFUNCEMITTER_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class V3_ਕ੍ਰਮ_CFunc_Emitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ਅਚਲ m_trigVscp;
    // Name component to add to function - must be unique
    ਅਚਲ std::string m_ਟੈਗ;
    // True if creating slow functions
    ਅਚਲ bool m_slow;
    // Whether to split functions
    ਅਚਲ bool m_ਵੰਡੋ = v3Global.opt.output_ਵੰਡੋ_CFuncs();
    // Size of code emitted so in the current function - for splitting
    ਮਾਪ_t m_ਮਾਪ = 0;
    // Maximum size of code per function
    ਅਚਲ ਮਾਪ_t m_ਵੰਡੋ_ਮਾਪ = []() -> ਮਾਪ_t {
        // Don't want to split with procCFuncs
        ਜੇ (v3Global.opt.profCFuncs()) ਵਾਪਸ std::numeric_limits<ਮਾਪ_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        ਜੇ (ਅਚਲ ਮਾਪ_t ਹੱਦਬੰਦੀ = v3Global.opt.output_ਵੰਡੋ_CFuncs()) ਵਾਪਸ ਹੱਦਬੰਦੀ - 1;
        ਵਾਪਸ std::numeric_limits<ਮਾਪ_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ਗੰਢ_Module*, std::string>, ਬਿਨਾਨਿਸ਼ਾਨੀ> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::ਸਦਿਸ਼<std::pair<AstCFunc*, ਅਚਲ ਕ੍ਰਮ_Domain*>> m_ਨਤੀਜਾ;

    // Create a unique name for a new function
    std::string cfuncName(File_ਸਤਰ* flp, AstScope* scopep, Ast_ਗੰਢ_Module* modp,
                          ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* lVtxp) {
        std::string name = "_" + m_ਟੈਗ;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::ਮੰਜ਼ਿਲ_string(m_funcNums[{modp, name}]++);
        ਜੇ (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        ਵਾਪਸ name;
    }

ਜਨਤਕ:
    // CONSTRUCTOR
    V3_ਕ੍ਰਮ_CFunc_Emitter(AstVarScope* trigVscp, ਅਚਲ std::string& ਟੈਗ, bool slow)
        : m_trigVscp{trigVscp}
        , m_ਟੈਗ{ਟੈਗ}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3_ਕ੍ਰਮ_CFunc_Emitter);
    VL_UNMOVABLE(V3_ਕ੍ਰਮ_CFunc_Emitter);

    // Force the creation of a new function
    ਖਾਲੀ forceNewFunction() {
        m_ਮਾਪ = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        ਅਚਲ ਕ੍ਰਮ_Domain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_ਨਤੀਜਾ) {
            AstCFunc* ਅਚਲ cfuncp = pair.ਪਹਿਲਾ;
            ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ domainp = pair.ਦੂਜਾ;
            // Create a new AstIf if the trigger is different
            ਜੇ (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_ਸਤਰ* ਅਚਲ flp = m_trigVscp->fileline();
                ਅਚਲ std::ਸੈੱਟ<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ਅਚਲ condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.ਸਿਰਾ()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ਅਚਲ callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtype_ਸੈੱਟ_Void();
            currp->ਜੋੜੋ_Thensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_ਨਤੀਜਾ.clear();
        forceNewFunction();
        // Return the list of statements
        ਵਾਪਸ stmtsp;
    }

    // Emit one logic vertex
    ਖਾਲੀ emitLogic(ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_ਗੰਢ* ਅਚਲ logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_ਗੰਢ_Procedure* ਅਚਲ procp = VN_CAST(logicp, ਗੰਢ_Procedure);

        // Some properties to consider
        ਅਚਲ bool suspendable = procp && procp->isSuspendable();
        ਅਚਲ bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ਅਚਲ bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        ਜੇ (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        ਜੇ (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        ਜੇ (!m_ਨਤੀਜਾ.ਖਾਲੀ_ਥਾਵਾਂ() && m_ਨਤੀਜਾ.ਪਿੱਠ().ਦੂਜਾ != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ਗੰਢ* ਅਚਲ headp = [&]() -> Ast_ਗੰਢ* {
            ਜੇ (!procp) ਵਾਪਸ logicp->unlink_Fr_ਪਿੱਠ();  // Not a procedure, handle as a unit
            Ast_ਗੰਢ* ਅਚਲ stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            ਵਾਪਸ stmtsp;
        }();
        // Process each statement in the list starting at headp
        ਲਈ (Ast_ਗੰਢ *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            ਜੇ (nextp) nextp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            // Split the function if too large, but don't split suspendable processes
            ਜੇ (!suspendable && m_ਮਾਪ >= m_ਵੰਡੋ_ਮਾਪ) forceNewFunction();
            // Create a new function if we don't have a current one
            ਜੇ (!m_funcp) {
                UASSERT_OBJ(!m_ਮਾਪ, currp, "Should have used forceNewFunction");
                File_ਸਤਰ* ਅਚਲ flp = currp->fileline();
                AstScope* ਅਚਲ scopep = lVtxp->scopep();
                Ast_ਗੰਢ_Module* ਅਚਲ modp = scopep->modp();
                ਅਚਲ std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                ਜੇ (needProcess) m_funcp->ਸੈੱਟ_Need_Process();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->ਜੋੜੋ_Blocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_ਨਤੀਜਾ.emplace_ਪਿੱਠ(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->ਜੋੜੋ_Stmtsp(currp);
            // If splitting, add in the size of the code we just added
            ਜੇ (m_ਵੰਡੋ) m_ਮਾਪ += currp->ਗੰਢ_ਗਿਣਤੀ();
        }
        // Put suspendable processes into individual functions on their own
        ਜੇ (suspendable) forceNewFunction();
    }
};

#ਜੇ_ਅੰਤ  // Guard
