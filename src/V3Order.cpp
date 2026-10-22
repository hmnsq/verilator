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
// V3Order's Transformations:
//
//  Compute near optimal scheduling of always/wire statements
//  Make a graph of the entire netlist
//
//      For seq logic
//          Add logic_sensitive_vertex for this list of SenItems
//              Add edge for each sensitive_var->logic_sensitive_vertex
//          For AlwaysPre's
//              Add vertex for this logic
//                  Add edge logic_sensitive_vertex->logic_vertex
//                  Add edge logic_consumed_var_PREVAR->logic_vertex
//                  Add edge logic_vertex->logic_generated_var (same as if comb)
//                  Add edge logic_vertex->generated_var_PREORDER
//                      Cutable dependency to attempt to order dlyed
//                      assignments to avoid saving state, thus we prefer
//                              a <= b ...      As the opposite order would
//                              b <= c ...      require the old value of b.
//                  Add edge consumed_var_POST->logic_vertex
//                      This prevents a consumer of the "early" value to be
//                      scheduled after we've changed to the next-cycle value
//          For Logic
//              Add vertex for this logic
//                  Add edge logic_sensitive_vertex->logic_vertex
//                  Add edge logic_generated_var_PREORDER->logic_vertex
//                      This ensures the AlwaysPre gets scheduled before this logic
//                  Add edge logic_vertex->consumed_var_PREVAR
//                  Add edge logic_vertex->consumed_var_POSTVAR
//                  Add edge logic_vertex->logic_generated_var (same as if comb)
//          For AlwaysPost's
//              Add vertex for this logic
//                  Add edge logic_sensitive_vertex->logic_vertex
//                  Add edge logic_consumed_var->logic_vertex (same as if comb)
//                  Add edge logic_vertex->logic_generated_var (same as if comb)
//                  Add edge consumed_var_POST->logic_vertex (same as if comb)
//
//      For comb logic
//          For comb logic
//              Add vertex for this logic
//              Add edge logic_consumed_var->logic_vertex
//              Add edge logic_vertex->logic_generated_var
//                  Mark it cutable, as circular logic may require
//                  the generated signal to become a primary input again.
//
//
//
//   Rank the graph starting at INPUTS (see V3Graph)
//
//   Visit the graph's logic vertices in ranked order
//      For all logic vertices with all inputs already ordered
//         Make ordered block for this module
//         For all ^^ in same domain
//              Move logic to ordered activation
//      When we have no more choices, we move to the next module
//      and make a new block.  Add that new activation block to the list of calls to make.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderInternal.h"
#ضمن "V3Sched.h"

#ضمن <memory>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

فراغ V3_ترتيب::ترتيب_ترتيب_گراف(ترتيب_گراف& گراف, ثابت std::string& ٽيگ) {
    // Dump data
    إذا (dump_گراف_Level()) گراف.dumpDotFilePrefixed(ٽيگ + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    گراف.acyclic(&V3_گراف_ڪنارو::followAlwaysTrue);
    إذا (dump_گراف_Level()) گراف.dumpDotFilePrefixed(ٽيگ + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    گراف.ترتيب();
    إذا (dump_گراف_Level()) گراف.dumpDotFilePrefixed(ٽيگ + "_orderg_order");
}

//######################################################################

AstCFunc* V3_ترتيب::ترتيب(ثابت std::ويڪٽر<V3Sched::LogicByScope*>& logic,  //
                         ثابت V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ثابت V3Sched::CovergroupRefBindings& cgRefBindings,
                         ثابت string& ٽيگ,  //
                         منطقي parallel,  //
                         منطقي slow,  //
                         ثابت ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    ثابت std::unique_ptr<ترتيب_گراف> گراف
        = build_ترتيب_گراف(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    ترتيب_ترتيب_گراف(*گراف, ٽيگ);
    // Assign sensitivity domains to combinational logic
    processDomains(*گراف, ٽيگ, externalTriggers);
    // Build the move graph
    ترتيب_Move_Dom_Scope::clear();
    ثابت std::unique_ptr<ترتيب_Move_گراف> moveGraphp = ترتيب_Move_گراف::build(*گراف);
    إذا (dump_گراف_Level() >= 9) moveGraphp->dumpDotFilePrefixed(ٽيگ + "_ordermv");

    // The ordered statements, if there are any
    Ast_ڳنڍ_Stmt* stmtsp = خالي_اشارو;
    إذا (!moveGraphp->خالي_جايون()) {
        إذا (parallel) {
            stmtsp = createParallel(*moveGraphp, ٽيگ, slow);
        } وإلا {
            stmtsp = createSerial(*moveGraphp, ٽيگ, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->خالي_جايون(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    ترتيب_Move_Dom_Scope::clear();

    // Dump data
    إذا (dump_گراف_Level()) گراف->dumpDotFilePrefixed(ٽيگ + "_orderg_done");

    // Dispose of the remnants of the inputs
    لكل (خودڪار* ثابت lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    إذا (!stmtsp) أرجع خالي_اشارو;

    // Create the result function
    AstNetlist* ثابت netlistp = v3Global.rootp();
    File_سٽ* ثابت flp = netlistp->fileline();
    AstCFunc* ثابت funcp = [&]() {
        AstScope* ثابت scopeTopp = netlistp->چوٽي_Scopep()->scopep();
        AstCFunc* ثابت resp = جديد AstCFunc{flp, "_eval_body__" + ٽيگ, scopeTopp, ""};
        resp->dontCombine(سچ);
        resp->isStatic(خطأ);
        resp->isLoose(سچ);
        resp->slow(slow);
        resp->isConst(خطأ);
        resp->decl_ذاتي(سچ);
        scopeTopp->جوڙيو_Blocksp(resp);
        أرجع resp;
    }();

    // Assemble the body
    funcp->جوڙيو_Stmtsp(stmtsp);

    //پورو
    أرجع funcp;
}
