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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ORDERCFUNCEMITTER_H_
#وصف_ڪريو VERILATOR_V3ORDERCFUNCEMITTER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3OrderGraph.h"
#ضمن "V3Sched.h"

#ضمن <limits>
#ضمن <map>
#ضمن <vector>

class V3_ترتيب_CFunc_Emitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ثابت m_trigVscp;
    // Name component to add to function - must be unique
    ثابت std::string m_ٽيگ;
    // True if creating slow functions
    ثابت bool m_slow;
    // Whether to split functions
    ثابت bool m_ورهايو = v3Global.opt.output_ورهايو_CFuncs();
    // Size of code emitted so in the current function - for splitting
    سائيز_t m_سائيز = 0;
    // Maximum size of code per function
    ثابت سائيز_t m_ورهايو_سائيز = []() -> سائيز_t {
        // Don't want to split with procCFuncs
        إذا (v3Global.opt.profCFuncs()) أرجع std::numeric_limits<سائيز_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        إذا (ثابت سائيز_t حد_بندي = v3Global.opt.output_ورهايو_CFuncs()) أرجع حد_بندي - 1;
        أرجع std::numeric_limits<سائيز_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ڳنڍ_Module*, std::string>, بنانشان> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::ويڪٽر<std::pair<AstCFunc*, ثابت ترتيب_Domain*>> m_نتيجو;

    // Create a unique name for a new function
    std::string cfuncName(File_سٽ* flp, AstScope* scopep, Ast_ڳنڍ_Module* modp,
                          ثابت ترتيب_Logic_راس* lVtxp) {
        std::string name = "_" + m_ٽيگ;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::منزل_string(m_funcNums[{modp, name}]++);
        إذا (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        أرجع name;
    }

عام:
    // CONSTRUCTOR
    V3_ترتيب_CFunc_Emitter(AstVarScope* trigVscp, ثابت std::string& ٽيگ, bool slow)
        : m_trigVscp{trigVscp}
        , m_ٽيگ{ٽيگ}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3_ترتيب_CFunc_Emitter);
    VL_UNMOVABLE(V3_ترتيب_CFunc_Emitter);

    // Force the creation of a new function
    فراغ forceNewFunction() {
        m_سائيز = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        ثابت ترتيب_Domain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        لكل (ثابت خودڪار& pair : m_نتيجو) {
            AstCFunc* ثابت cfuncp = pair.پهريون;
            ثابت ترتيب_Domain* ثابت domainp = pair.ٻيو;
            // Create a new AstIf if the trigger is different
            إذا (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_سٽ* ثابت flp = m_trigVscp->fileline();
                ثابت std::سيٽ<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ثابت condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.پڇاڙي()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ثابت callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtype_سيٽ_Void();
            currp->جوڙيو_Thensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_نتيجو.clear();
        forceNewFunction();
        // Return the list of statements
        أرجع stmtsp;
    }

    // Emit one logic vertex
    فراغ emitLogic(ثابت ترتيب_Logic_راس* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ثابت ترتيب_Domain* ثابت domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_ڳنڍ* ثابت logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_ڳنڍ_Procedure* ثابت procp = VN_CAST(logicp, ڳنڍ_Procedure);

        // Some properties to consider
        ثابت bool suspendable = procp && procp->isSuspendable();
        ثابت bool needProcess = procp && procp->needProcess();
        // TODO: This is a bit muddy: 'initial forever @(posedge clk) begin ... end' is a fancy
        //       way of saying always @(posedge clk), so it might be quite hot...
        //       Also, if m_funcp is slow, but this one isn't we should force a new function
        ثابت bool slow = m_slow && !(suspendable && VN_IS(procp, Always));

        // Put suspendable processes into individual functions on their own
        إذا (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        إذا (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        إذا (!m_نتيجو.خالي_جايون() && m_نتيجو.پٺ().ٻيو != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ڳنڍ* ثابت headp = [&]() -> Ast_ڳنڍ* {
            إذا (!procp) أرجع logicp->unlink_Fr_پٺ();  // Not a procedure, handle as a unit
            Ast_ڳنڍ* ثابت stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_پٺ_With_ايندڙ();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            أرجع stmtsp;
        }();
        // Process each statement in the list starting at headp
        لكل (Ast_ڳنڍ *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            إذا (nextp) nextp->unlink_Fr_پٺ_With_ايندڙ();
            // Split the function if too large, but don't split suspendable processes
            إذا (!suspendable && m_سائيز >= m_ورهايو_سائيز) forceNewFunction();
            // Create a new function if we don't have a current one
            إذا (!m_funcp) {
                UASSERT_OBJ(!m_سائيز, currp, "Should have used forceNewFunction");
                File_سٽ* ثابت flp = currp->fileline();
                AstScope* ثابت scopep = lVtxp->scopep();
                Ast_ڳنڍ_Module* ثابت modp = scopep->modp();
                ثابت std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                إذا (needProcess) m_funcp->سيٽ_Need_Process();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->جوڙيو_Blocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_نتيجو.emplace_پٺ(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->جوڙيو_Stmtsp(currp);
            // If splitting, add in the size of the code we just added
            إذا (m_ورهايو) m_سائيز += currp->ڳنڍ_ڳڻپ();
        }
        // Put suspendable processes into individual functions on their own
        إذا (suspendable) forceNewFunction();
    }
};

#جيڪڏهن_پورو  // Guard
