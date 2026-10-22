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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3OrderInternal.h"
#समावेश "V3Sched.h"

#समावेश <memory>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

व्योम V3Order::orderOrderGraph(OrderGraph& graph, स्थिर std::string& tag) {
    // Dump data
    अगर (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    graph.acyclic(&V3GraphEdge::followAlwaysTrue);
    अगर (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    graph.order();
    अगर (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_order");
}

//######################################################################

AstCFunc* V3Order::order(स्थिर std::vector<V3Sched::LogicByScope*>& logic,  //
                         स्थिर V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         स्थिर V3Sched::CovergroupRefBindings& cgRefBindings,
                         स्थिर string& tag,  //
                         bool parallel,  //
                         bool slow,  //
                         स्थिर ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    स्थिर std::unique_ptr<OrderGraph> graph
        = buildOrderGraph(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    orderOrderGraph(*graph, tag);
    // Assign sensitivity domains to combinational logic
    processDomains(*graph, tag, externalTriggers);
    // Build the move graph
    OrderMoveDomScope::सफा();
    स्थिर std::unique_ptr<OrderMoveGraph> moveGraphp = OrderMoveGraph::build(*graph);
    अगर (dumpGraphLevel() >= 9) moveGraphp->dumpDotFilePrefixed(tag + "_ordermv");

    // The ordered statements, if there are any
    Ast_गाँठ_Stmt* stmtsp = nullptr;
    अगर (!moveGraphp->empty()) {
        अगर (parallel) {
            stmtsp = createParallel(*moveGraphp, tag, slow);
        } अन्यथा {
            stmtsp = createSerial(*moveGraphp, tag, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->empty(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    OrderMoveDomScope::सफा();

    // Dump data
    अगर (dumpGraphLevel()) graph->dumpDotFilePrefixed(tag + "_orderg_done");

    // Dispose of the remnants of the inputs
    क्रम (स्वतः* स्थिर lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    अगर (!stmtsp) वापस nullptr;

    // Create the result function
    AstNetlist* स्थिर netlistp = v3Global.rootp();
    File_पंक्ति* स्थिर flp = netlistp->fileline();
    AstCFunc* स्थिर funcp = [&]() {
        AstScope* स्थिर scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
        AstCFunc* स्थिर resp = नया AstCFunc{flp, "_eval_body__" + tag, scopeTopp, ""};
        resp->dontCombine(true);
        resp->isStatic(false);
        resp->isLoose(true);
        resp->slow(slow);
        resp->isConst(false);
        resp->declPrivate(true);
        scopeTopp->addBlocksp(resp);
        वापस resp;
    }();

    // Assemble the body
    funcp->addStmtsp(stmtsp);

    // Done
    वापस funcp;
}
