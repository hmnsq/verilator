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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3OrderInternal.h"
#அந்தர்பாவ "V3Sched.h"

#அந்தர்பாவ <memory>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

வெறுமை V3Order::orderOrderGraph(OrderGraph& graph, மாறிலி std::string& tag) {
    // Dump data
    எனில் (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    graph.acyclic(&V3GraphEdge::followAlwaysTrue);
    எனில் (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    graph.order();
    எனில் (dumpGraphLevel()) graph.dumpDotFilePrefixed(tag + "_orderg_order");
}

//######################################################################

AstCFunc* V3Order::order(மாறிலி std::vector<V3Sched::LogicByScope*>& logic,  //
                         மாறிலி V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         மாறிலி V3Sched::CovergroupRefBindings& cgRefBindings,
                         மாறிலி string& tag,  //
                         தருக்க parallel,  //
                         தருக்க slow,  //
                         மாறிலி ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    மாறிலி std::unique_ptr<OrderGraph> graph
        = buildOrderGraph(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    orderOrderGraph(*graph, tag);
    // Assign sensitivity domains to combinational logic
    processDomains(*graph, tag, externalTriggers);
    // Build the move graph
    OrderMoveDomScope::clear();
    மாறிலி std::unique_ptr<OrderMoveGraph> moveGraphp = OrderMoveGraph::build(*graph);
    எனில் (dumpGraphLevel() >= 9) moveGraphp->dumpDotFilePrefixed(tag + "_ordermv");

    // The ordered statements, if there are any
    Ast_முனை_Stmt* stmtsp = nullptr;
    எனில் (!moveGraphp->empty()) {
        எனில் (parallel) {
            stmtsp = createParallel(*moveGraphp, tag, slow);
        } இல்லையெனில் {
            stmtsp = createSerial(*moveGraphp, tag, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->empty(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    OrderMoveDomScope::clear();

    // Dump data
    எனில் (dumpGraphLevel()) graph->dumpDotFilePrefixed(tag + "_orderg_done");

    // Dispose of the remnants of the inputs
    க்கு (auto* மாறிலி lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    எனில் (!stmtsp) திரும்பு nullptr;

    // Create the result function
    AstNetlist* மாறிலி netlistp = v3Global.rootp();
    File_வரி* மாறிலி flp = netlistp->fileline();
    AstCFunc* மாறிலி funcp = [&]() {
        AstScope* மாறிலி scopeTopp = netlistp->உச்சி_Scopep()->scopep();
        AstCFunc* மாறிலி resp = புதிய AstCFunc{flp, "_eval_body__" + tag, scopeTopp, ""};
        resp->dontCombine(உண்மை);
        resp->isStatic(பொய்);
        resp->isLoose(உண்மை);
        resp->slow(slow);
        resp->isConst(பொய்);
        resp->declPrivate(உண்மை);
        scopeTopp->addBlocksp(resp);
        திரும்பு resp;
    }();

    // Assemble the body
    funcp->addStmtsp(stmtsp);

    // Done
    திரும்பு funcp;
}
