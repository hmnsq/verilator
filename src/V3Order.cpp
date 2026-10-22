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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderInternal.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <memory>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

शून्य V3_क्रमः::क्रमः_क्रमः_Graph(क्रमः_Graph& graph, ध्रुव std::string& चिह्नकम्) {
    // Dump data
    यदि (dumpGraphLevel()) graph.dumpDotFilePrefixed(चिह्नकम् + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    graph.acyclic(&V3GraphEdge::followAlwaysTrue);
    यदि (dumpGraphLevel()) graph.dumpDotFilePrefixed(चिह्नकम् + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    graph.क्रमः();
    यदि (dumpGraphLevel()) graph.dumpDotFilePrefixed(चिह्नकम् + "_orderg_order");
}

//######################################################################

AstCFunc* V3_क्रमः::क्रमः(ध्रुव std::सदिशम्<V3Sched::LogicByScope*>& logic,  //
                         ध्रुव V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ध्रुव V3Sched::CovergroupRefBindings& cgRefBindings,
                         ध्रुव string& चिह्नकम्,  //
                         तार्किक parallel,  //
                         तार्किक slow,  //
                         ध्रुव ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    ध्रुव std::unique_ptr<क्रमः_Graph> graph
        = build_क्रमः_Graph(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    क्रमः_क्रमः_Graph(*graph, चिह्नकम्);
    // Assign sensitivity domains to combinational logic
    processDomains(*graph, चिह्नकम्, externalTriggers);
    // Build the move graph
    क्रमः_Move_Dom_Scope::clear();
    ध्रुव std::unique_ptr<क्रमः_Move_Graph> moveGraphp = क्रमः_Move_Graph::build(*graph);
    यदि (dumpGraphLevel() >= 9) moveGraphp->dumpDotFilePrefixed(चिह्नकम् + "_ordermv");

    // The ordered statements, if there are any
    Ast_ग्रन्थिः_Stmt* stmtsp = शून्यसूचकः;
    यदि (!moveGraphp->रिक्तानि()) {
        यदि (parallel) {
            stmtsp = createParallel(*moveGraphp, चिह्नकम्, slow);
        } अन्यथा {
            stmtsp = createSerial(*moveGraphp, चिह्नकम्, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->रिक्तानि(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    क्रमः_Move_Dom_Scope::clear();

    // Dump data
    यदि (dumpGraphLevel()) graph->dumpDotFilePrefixed(चिह्नकम् + "_orderg_done");

    // Dispose of the remnants of the inputs
    प्रति (स्वचालित* ध्रुव lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    यदि (!stmtsp) प्रत्यागम शून्यसूचकः;

    // Create the result function
    AstNetlist* ध्रुव netlistp = v3Global.rootp();
    File_पङ्क्तिः* ध्रुव flp = netlistp->fileline();
    AstCFunc* ध्रुव funcp = [&]() {
        AstScope* ध्रुव scopeTopp = netlistp->शिखरम्_Scopep()->scopep();
        AstCFunc* ध्रुव resp = नूतन AstCFunc{flp, "_eval_body__" + चिह्नकम्, scopeTopp, ""};
        resp->dontCombine(सत्य);
        resp->isStatic(असत्य);
        resp->isLoose(सत्य);
        resp->slow(slow);
        resp->isConst(असत्य);
        resp->decl_वैयक्तिकम्(सत्य);
        scopeTopp->सङ्कलय_Blocksp(resp);
        प्रत्यागम resp;
    }();

    // Assemble the body
    funcp->सङ्कलय_Stmtsp(stmtsp);

    //समाप्तम्
    प्रत्यागम funcp;
}
