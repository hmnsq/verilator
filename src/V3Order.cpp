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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3OrderInternal.h"
#અન્તર્ભાવ "V3Sched.h"

#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

શૂન્ય V3Order::orderOrderGraph(OrderGraph& graph, અચલ std::string& tag) {
    // Dump data
    જો (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    graph.acyclic(&V3GraphEdge::followAlwaysTrue);
    જો (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    graph.order();
    જો (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_order");
}

//######################################################################

AstCFunc* V3Order::order(અચલ std::vector<V3Sched::LogicByScope*>& logic,  //
                         અચલ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         અચલ V3Sched::CovergroupRefBindings& cgRefBindings,
                         અચલ string& tag,  //
                         તાર્કિક parallel,  //
                         તાર્કિક slow,  //
                         અચલ ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    અચલ std::unique_ptr<OrderGraph> graph
        = buildOrderGraph(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    orderOrderGraph(*graph, tag);
    // Assign sensitivity domains to combinational logic
    processDomains(*graph, tag, externalTriggers);
    // Build the move graph
    OrderMoveDomScope::clear();
    અચલ std::unique_ptr<OrderMoveGraph> moveGraphp = OrderMoveGraph::build(*graph);
    જો (dumpGraphLevel() >= 9) moveGraphp->dumpDotFilePrefixed(tag + "_ordermv");

    // The ordered statements, if there are any
    Ast_ગાંઠ_Stmt* stmtsp = nullptr;
    જો (!moveGraphp->empty()) {
        જો (parallel) {
            stmtsp = createParallel(*moveGraphp, tag, slow);
        } નહીંતર {
            stmtsp = createSerial(*moveGraphp, tag, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->empty(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    OrderMoveDomScope::clear();

    // Dump data
    જો (dumpGraphLevel()) graph->dumpDotFilePrefixed(tag + "_orderg_done");

    // Dispose of the remnants of the inputs
    માટે (auto* અચલ lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    જો (!stmtsp) પાછા nullptr;

    // Create the result function
    AstNetlist* અચલ netlistp = v3Global.rootp();
    File_લીટી* અચલ flp = netlistp->fileline();
    AstCFunc* અચલ funcp = [&]() {
        AstScope* અચલ scopeTopp = netlistp->ટોચ_Scopep()->scopep();
        AstCFunc* અચલ resp = નવું AstCFunc{flp, "_eval_body__" + tag, scopeTopp, ""};
        resp->dontCombine(સાચું);
        resp->isStatic(ખોટું);
        resp->isLoose(સાચું);
        resp->slow(slow);
        resp->isConst(ખોટું);
        resp->declPrivate(સાચું);
        scopeTopp->addBlocksp(resp);
        પાછા resp;
    }();

    // Assemble the body
    funcp->addStmtsp(stmtsp);

    // Done
    પાછા funcp;
}
