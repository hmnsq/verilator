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

#यदि_अपरिभाषितम् VERILATOR_V3ORDERCFUNCEMITTER_H_
#परिभाषय VERILATOR_V3ORDERCFUNCEMITTER_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3OrderGraph.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <map>
#अन्तर्भाव <vector>

class V3_क्रमः_CFunc_Emitter final {
    // The trigger vector the ordered logic is sensitive to
    AstVarScope* ध्रुव m_trigVscp;
    // Name component to add to function - must be unique
    ध्रुव std::string m_चिह्नकम्;
    // True if creating slow functions
    ध्रुव bool m_slow;
    // Whether to split functions
    ध्रुव bool m_विभज = v3Global.opt.output_विभज_CFuncs();
    // Size of code emitted so in the current function - for splitting
    आकार_t m_आकार = 0;
    // Maximum size of code per function
    ध्रुव आकार_t m_विभज_आकार = []() -> आकार_t {
        // Don't want to split with procCFuncs
        यदि (v3Global.opt.profCFuncs()) प्रत्यागम std::numeric_limits<आकार_t>::max();
        // Otherwise split at the specified size (- 1, so we can do >= comparison)
        यदि (ध्रुव आकार_t सीमा = v3Global.opt.output_विभज_CFuncs()) प्रत्यागम सीमा - 1;
        प्रत्यागम std::numeric_limits<आकार_t>::max();
    }();
    // Current function being populated
    AstCFunc* m_funcp = nullptr;
    // Function ordinals to ensure unique names
    std::map<std::pair<Ast_ग्रन्थिः_Module*, std::string>, अचिह्नित> m_funcNums;
    // The resulting ordered CFuncs with the trigger conditions needed to call them
    std::सदिशम्<std::pair<AstCFunc*, ध्रुव क्रमः_Domain*>> m_परिणाम;

    // Create a unique name for a new function
    std::string cfuncName(File_पङ्क्तिः* flp, AstScope* scopep, Ast_ग्रन्थिः_Module* modp,
                          ध्रुव क्रमः_Logic_Vertex* lVtxp) {
        std::string name = "_" + m_चिह्नकम्;
        name += lVtxp->hybridp() ? "_hybrid" : lVtxp->comb() ? "_comb" : "_sequent";
        name += "__" + scopep->nameDotless();
        name += "__" + std::गन्तव्यम्_string(m_funcNums[{modp, name}]++);
        यदि (v3Global.opt.profCFuncs()) name += "__PROF__" + flp->profileFuncname();
        प्रत्यागम name;
    }

सार्वजनिक:
    // CONSTRUCTOR
    V3_क्रमः_CFunc_Emitter(AstVarScope* trigVscp, ध्रुव std::string& चिह्नकम्, bool slow)
        : m_trigVscp{trigVscp}
        , m_चिह्नकम्{चिह्नकम्}
        , m_slow{slow} {}
    VL_UNCOPYABLE(V3_क्रमः_CFunc_Emitter);
    VL_UNMOVABLE(V3_क्रमः_CFunc_Emitter);

    // Force the creation of a new function
    शून्य forceNewFunction() {
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
        ध्रुव क्रमः_Domain* पूर्व_Domainp = nullptr;
        // Call each function under an AstIf that checks for the trigger condition
        प्रति (ध्रुव स्वचालित& pair : m_परिणाम) {
            AstCFunc* ध्रुव cfuncp = pair.first;
            ध्रुव क्रमः_Domain* ध्रुव domainp = pair.second;
            // Create a new AstIf if the trigger is different
            यदि (domainp != पूर्व_Domainp) {
                पूर्व_Domainp = domainp;
                File_पङ्क्तिः* ध्रुव flp = m_trigVscp->fileline();
                ध्रुव std::समुच्चयः<uint32_t>& triggers = domainp->triggers();
                AstTriggered* ध्रुव condp
                    = new AstTriggered{flp, m_trigVscp, {triggers.begin(), triggers.अन्तः_2()}};
                currp = new AstIf{flp, condp};
                stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtsp, currp);
            }
            // Call function when triggered
            AstCCall* ध्रुव callp = new AstCCall{cfuncp->fileline(), cfuncp};
            callp->dtype_समुच्चयः_Void();
            currp->सङ्कलय_Thensp(callp->makeStmt());
        }
        // Result is now spent, reset the emitter state
        m_परिणाम.clear();
        forceNewFunction();
        // Return the list of statements
        प्रत्यागम stmtsp;
    }

    // Emit one logic vertex
    शून्य emitLogic(ध्रुव क्रमः_Logic_Vertex* lVtxp) {
        // Sensitivity domain of logic we are emitting
        ध्रुव क्रमः_Domain* ध्रुव domainp = lVtxp->domainp();
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
        यदि (suspendable) forceNewFunction();
        // When profCFuncs, create a new function for each logic vertex
        यदि (v3Global.opt.profCFuncs()) forceNewFunction();
        // If the new domain is different, force a new function as it needs to
        // be called under different trigger conditions
        यदि (!m_परिणाम.रिक्तानि() && m_परिणाम.पृष्ठभागः().second != domainp) forceNewFunction();

        // Process procedures per statement, so we can split CFuncs within procedures.
        // Everything else is handled as a unit.
        Ast_ग्रन्थिः* ध्रुव headp = [&]() -> Ast_ग्रन्थिः* {
            यदि (!procp) प्रत्यागम logicp->unlink_Fr_पृष्ठभागः();  // Not a procedure, handle as a unit
            Ast_ग्रन्थिः* ध्रुव stmtsp = procp->stmtsp();
            UASSERT_OBJ(stmtsp, procp, "Empty process should have been deleted earlier");
            stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            // 'procp' is no longer needed but the OrderGraph still references
            // it (for debug dump). It will get deleted when deleting the
            // AstActives in V3Order.
            प्रत्यागम stmtsp;
        }();
        // Process each statement in the list starting at headp
        प्रति (Ast_ग्रन्थिः *currp = headp, *nextp; currp; currp = nextp) {
            nextp = currp->nextp();
            // Unlink the current statement from the next statement (if any)
            यदि (nextp) nextp->unlink_Fr_पृष्ठभागः_With_अग्र();
            // Split the function if too large, but don't split suspendable processes
            यदि (!suspendable && m_आकार >= m_विभज_आकार) forceNewFunction();
            // Create a new function if we don't have a current one
            यदि (!m_funcp) {
                UASSERT_OBJ(!m_आकार, currp, "Should have used forceNewFunction");
                File_पङ्क्तिः* ध्रुव flp = currp->fileline();
                AstScope* ध्रुव scopep = lVtxp->scopep();
                Ast_ग्रन्थिः_Module* ध्रुव modp = scopep->modp();
                ध्रुव std::string name = cfuncName(flp, scopep, modp, lVtxp);
                m_funcp = new AstCFunc{flp, name, scopep, suspendable ? "VlCoroutine" : ""};
                यदि (needProcess) m_funcp->समुच्चयः_Need_Process();
                m_funcp->isStatic(false);
                m_funcp->isLoose(true);
                m_funcp->slow(slow);
                scopep->सङ्कलय_Blocksp(m_funcp);
                // Record function and sensitivity to call it with
                m_परिणाम.emplace_पृष्ठभागः(m_funcp, domainp);
            }
            // Add the code to the current function
            m_funcp->सङ्कलय_Stmtsp(currp);
            // If splitting, add in the size of the code we just added
            यदि (m_विभज) m_आकार += currp->ग्रन्थिः_गणनम्();
        }
        // Put suspendable processes into individual functions on their own
        यदि (suspendable) forceNewFunction();
    }
};

#यदि_अन्तः  // Guard
