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

#اگر_تعریف_نشده VERILATOR_V3ORDERCFUNCEMITTER_H_
#تعریف_کن VERILATOR_V3ORDERCFUNCEMITTER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3OrderGraph.h"
#ضمن "V3Sched.h"

#ضمن <limits>
#ضمن <map>
#ضمن <vector>

class V3_ترتیب_CFunc_Emitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ثابت m_trigVscp;
    // Name component to add to function - must be unique
    ثابت std::string m_برچسب;
    // True if creating slow functions
    ثابت bool m_slow;
    // Whether to split functions
    ثابت bool m_تقسیم = v3Global.opt.output_تقسیم_CFuncs();
    // Size of code emitted so in the current function - for splitting
    اندازه_t m_اندازه = 0;
    // Maximum size of code per function
    ثابت اندازه_t m_تقسیم_اندازه = []() -> اندازه_t {
        // Don't want to split with procCFuncs
        إذا (v3Global.opt.profCFuncs()) أرجع std::numeric_limits<اندازه_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        إذا (ثابت اندازه_t حد = v3Global.opt.output_تقسیم_CFuncs()) أرجع حد - 1;
        أرجع std::numeric_limits<اندازه_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_گره_Module*, std::string>, بیعلامت> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::وکتور<std::pair<AstCFunc*, ثابت ترتیب_Domain*>> m_نتیجه;

    // Create a unique name for a new function
    std::string cfuncName(File_سطر* flp, AstScope* scopep, Ast_گره_Module* modp,
                          ثابت ترتیب_Logic_رأس* lVtxp) {
        std::string name = "_" + m_برچسب;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::مقصد_string(m_funcNums[{modp, name}]++);
        إذا (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        أرجع name;
    }

عام:
    // CONSTRUCTOR
    V3_ترتیب_CFunc_Emitter(AstVarScope* trigVscp, ثابت std::string& برچسب, bool slow)
        : m_trigVscp{trigVscp}
        , m_برچسب{برچسب}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3_ترتیب_CFunc_Emitter);
    VL_UNMOVABLE(V3_ترتیب_CFunc_Emitter);

    // Force the creation of a new function
    فراغ forceNewFunction() {
        m_اندازه = 0;
        m_funcp = nullptr;
    }

    // Retrieve list of statements which when executed will call the constructed functions
    AstIf* getStmts() {
        // The resulting list of statements we are constructing here
        AstIf* stmtsp = nullptr;
        // Current AstIf doing the trigger condition check
        AstIf* currp = nullptr;
        // The domain of the previously emitted logic
        ثابت ترتیب_Domain* prevDomainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        لكل (ثابت خودکار& pair : m_نتیجه) {
            AstCFunc* ثابت cfuncp = pair.اولی;
            ثابت ترتیب_Domain* ثابت domainp = pair.دومی;
            // Create a new AstIf if the trigger is different
            إذا (domainp != prevDomainp) {
                prevDomainp = domainp;
                File_سطر* ثابت flp = m_trigVscp->fileline();
                ثابت std::مجموعه<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ثابت condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.انتها()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ثابت callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtype_مجموعه_Void();
            currp->جمع_کن_Thensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_نتیجه.clear();
        forceNewFunction();
        // Return the list of statements
        أرجع stmtsp;
    }

    // Emit one logic vertex
    فراغ emitLogic(ثابت ترتیب_Logic_رأس* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ثابت ترتیب_Domain* ثابت domainp = lVtxp->domainp();
        // We are move the logic into a CFunc
        Ast_گره* ثابت logicp = lVtxp->nodep();
        // If the logic is a procedure, we need to do a few special things
        Ast_گره_Procedure* ثابت procp = VN_CAST(logicp, گره_Procedure);

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
        إذا (!m_نتیجه.خالی() && m_نتیجه.پشت().دومی != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_گره* ثابت headp = [&]() -> Ast_گره* {
            إذا (!procp) أرجع logicp->unlink_Fr_پشت();  // Not a procedure, handle as a unit
            Ast_گره* ثابت stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_پشت_With_بعدی();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            أرجع stmtsp;
        }();
        // Process each statement in the list starting at headp
        لكل (Ast_گره *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            إذا (nextp) nextp->unlink_Fr_پشت_With_بعدی();
            // Split the function if too large, but don't split suspendable processes
            إذا (!suspendable && m_اندازه >= m_تقسیم_اندازه) forceNewFunction();
            // Create a new function if we don't have a current one
            إذا (!m_funcp) {
                UASSERT_OBJ(!m_اندازه, currp, "Should have used forceNewFunction");
                File_سطر* ثابت flp = currp->fileline();
                AstScope* ثابت scopep = lVtxp->scopep();
                Ast_گره_Module* ثابت modp = scopep->modp();
                ثابت std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                إذا (needProcess) m_funcp->مجموعه_Need_Process();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->جمع_کن_Blocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_نتیجه.emplace_پشت(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->جمع_کن_Stmtsp(currp);
            // If splitting, add in the size of the code we just added
            إذا (m_تقسیم) m_اندازه += currp->گره_شمار();
        }
        // Put suspendable processes into individual functions on their own
        إذا (suspendable) forceNewFunction();
    }
};

#پایان_اگر  // Guard
