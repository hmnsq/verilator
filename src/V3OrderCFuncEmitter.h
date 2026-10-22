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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ORDERCFUNCEMITTER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ORDERCFUNCEMITTER_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <vector>

class V3_ಕ್ರಮ_CFunc_Emitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ಸ್ಥಿರಾಂಕ m_trigVscp;
    // Name component to add to function - must be unique
    ಸ್ಥಿರಾಂಕ std::string m_ಟ್ಯಾಗ್;
    // True if creating slow functions
    ಸ್ಥಿರಾಂಕ bool m_slow;
    // Whether to split functions
    ಸ್ಥಿರಾಂಕ bool m_ವಿಭಜಿಸು = v3Global.opt.output_ವಿಭಜಿಸು_CFuncs();
    // Size of code emitted so in the current function - for splitting
    ಗಾತ್ರ_t m_ಗಾತ್ರ = 0;
    // Maximum size of code per function
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ವಿಭಜಿಸು_ಗಾತ್ರ = []() -> ಗಾತ್ರ_t {
        // Don't want to split with procCFuncs
        ಆದರೆ (v3Global.opt.profCFuncs()) ಹಿಂತಿರುಗಿ std::numeric_limits<ಗಾತ್ರ_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಎಲ್ಲೆ = v3Global.opt.output_ವಿಭಜಿಸು_CFuncs()) ಹಿಂತಿರುಗಿ ಎಲ್ಲೆ - 1;
        ಹಿಂತಿರುಗಿ std::numeric_limits<ಗಾತ್ರ_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ಗಂಟು_Module*, std::string>, ಚಿಹ್ನೆರಹಿತ> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::ಸದಿಶ<std::pair<AstCFunc*, ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain*>> m_ಫಲಿತಾಂಶ;

    // Create a unique name for a new function
    std::string cfuncName(File_ಸಾಲು* flp, AstScope* scopep, Ast_ಗಂಟು_Module* modp,
                          ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* lVtxp) {
        std::string name = "_" + m_ಟ್ಯಾಗ್;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::ಗಮ್ಯ_string(m_funcNums[{modp, name}]++);
        ಆದರೆ (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        ಹಿಂತಿರುಗಿ name;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    V3_ಕ್ರಮ_CFunc_Emitter(AstVarScope* trigVscp, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್, bool slow)
        : m_trigVscp{trigVscp}
        , m_ಟ್ಯಾಗ್{ಟ್ಯಾಗ್}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3_ಕ್ರಮ_CFunc_Emitter);
    VL_UNMOVABLE(V3_ಕ್ರಮ_CFunc_Emitter);

    // Force the creation of a new function
    ಶೂನ್ಯ forceNewFunction() {
        m_ಗಾತ್ರ = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_ಫಲಿತಾಂಶ) {
            AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = pair.ಮೊದಲ;
            ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ domainp = pair.ಎರಡನೇ;
            // Create a new AstIf if the trigger is different
            ಆದರೆ (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_trigVscp->fileline();
                ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ಸ್ಥಿರಾಂಕ condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.ಕೊನೆ()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ಸ್ಥಿರಾಂಕ callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtype_ಗಣ_Void();
            currp->ಕೂಡಿಸು_Thensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_ಫಲಿತಾಂಶ.clear();
        forceNewFunction();
        // Return the list of statements
        ಹಿಂತಿರುಗಿ stmtsp;
    }

    // Emit one logic vertex
    ಶೂನ್ಯ emitLogic(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ procp = VN_CAST(logicp, ಗಂಟು_Procedure);

        // Some properties to consider
        ಸ್ಥಿರಾಂಕ bool suspendable = procp && procp->isSuspendable();
        ಸ್ಥಿರಾಂಕ bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ಸ್ಥಿರಾಂಕ bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        ಆದರೆ (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        ಆದರೆ (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        ಆದರೆ (!m_ಫಲಿತಾಂಶ.ಖಾಲಿಗಳು() && m_ಫಲಿತಾಂಶ.ಹಿಂಭಾಗ().ಎರಡನೇ != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ headp = [&]() -> Ast_ಗಂಟು* {
            ಆದರೆ (!procp) ಹಿಂತಿರುಗಿ logicp->unlink_Fr_ಹಿಂಭಾಗ();  // Not a procedure, handle as a unit
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            ಹಿಂತಿರುಗಿ stmtsp;
        }();
        // Process each statement in the list starting at headp
        ಗಾಗಿ (Ast_ಗಂಟು *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            ಆದರೆ (nextp) nextp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            // Split the function if too large, but don't split suspendable processes
            ಆದರೆ (!suspendable && m_ಗಾತ್ರ >= m_ವಿಭಜಿಸು_ಗಾತ್ರ) forceNewFunction();
            // Create a new function if we don't have a current one
            ಆದರೆ (!m_funcp) {
                UASSERT_OBJ(!m_ಗಾತ್ರ, currp, "Should have used forceNewFunction");
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = currp->fileline();
                AstScope* ಸ್ಥಿರಾಂಕ scopep = lVtxp->scopep();
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = scopep->modp();
                ಸ್ಥಿರಾಂಕ std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                ಆದರೆ (needProcess) m_funcp->ಗಣ_Need_Process();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->ಕೂಡಿಸು_Blocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->ಕೂಡಿಸು_Stmtsp(currp);
            // If splitting, add in the size of the code we just added
            ಆದರೆ (m_ವಿಭಜಿಸು) m_ಗಾತ್ರ += currp->ಗಂಟು_ಎಣಿಕೆ();
        }
        // Put suspendable processes into individual functions on their own
        ಆದರೆ (suspendable) forceNewFunction();
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
