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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3OrderInternal.h"
#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ <memory>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

శూన్య V3_క్రమం::క్రమం_క్రమం_లేఖాచిత్రం(క్రమం_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి std::string& ట్యాగ్) {
    // Dump data
    ఒకవేళ (dump_లేఖాచిత్రం_Level()) లేఖాచిత్రం.dumpDotFilePrefixed(ట్యాగ్ + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    లేఖాచిత్రం.acyclic(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
    ఒకవేళ (dump_లేఖాచిత్రం_Level()) లేఖాచిత్రం.dumpDotFilePrefixed(ట్యాగ్ + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    లేఖాచిత్రం.క్రమం();
    ఒకవేళ (dump_లేఖాచిత్రం_Level()) లేఖాచిత్రం.dumpDotFilePrefixed(ట్యాగ్ + "_orderg_order");
}

//######################################################################

AstCFunc* V3_క్రమం::క్రమం(స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& logic,  //
                         స్థిరరాశి V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         స్థిరరాశి V3Sched::CovergroupRefBindings& cgRefBindings,
                         స్థిరరాశి string& ట్యాగ్,  //
                         తార్కిక parallel,  //
                         తార్కిక slow,  //
                         స్థిరరాశి ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    స్థిరరాశి std::unique_ptr<క్రమం_లేఖాచిత్రం> లేఖాచిత్రం
        = build_క్రమం_లేఖాచిత్రం(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    క్రమం_క్రమం_లేఖాచిత్రం(*లేఖాచిత్రం, ట్యాగ్);
    // Assign sensitivity domains to combinational logic
    processDomains(*లేఖాచిత్రం, ట్యాగ్, externalTriggers);
    // Build the move graph
    క్రమం_Move_Dom_Scope::clear();
    స్థిరరాశి std::unique_ptr<క్రమం_Move_లేఖాచిత్రం> moveGraphp = క్రమం_Move_లేఖాచిత్రం::build(*లేఖాచిత్రం);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) moveGraphp->dumpDotFilePrefixed(ట్యాగ్ + "_ordermv");

    // The ordered statements, if there are any
    Ast_కణుపు_Stmt* stmtsp = శూన్యసూచిక;
    ఒకవేళ (!moveGraphp->ఖాళీలు()) {
        ఒకవేళ (parallel) {
            stmtsp = createParallel(*moveGraphp, ట్యాగ్, slow);
        } లేకపోతే {
            stmtsp = createSerial(*moveGraphp, ట్యాగ్, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->ఖాళీలు(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    క్రమం_Move_Dom_Scope::clear();

    // Dump data
    ఒకవేళ (dump_లేఖాచిత్రం_Level()) లేఖాచిత్రం->dumpDotFilePrefixed(ట్యాగ్ + "_orderg_done");

    // Dispose of the remnants of the inputs
    కొరకు (స్వయంచాలక* స్థిరరాశి lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    ఒకవేళ (!stmtsp) తిరిగి శూన్యసూచిక;

    // Create the result function
    AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
    File_పంక్తి* స్థిరరాశి flp = netlistp->fileline();
    AstCFunc* స్థిరరాశి funcp = [&]() {
        AstScope* స్థిరరాశి scopeTopp = netlistp->శిఖరం_Scopep()->scopep();
        AstCFunc* స్థిరరాశి resp = కొత్త AstCFunc{flp, "_eval_body__" + ట్యాగ్, scopeTopp, ""};
        resp->dontCombine(నిజం);
        resp->isStatic(అబద్ధం);
        resp->isLoose(నిజం);
        resp->slow(slow);
        resp->isConst(అబద్ధం);
        resp->decl_వ్యక్తిగత(నిజం);
        scopeTopp->కూడు_Blocksp(resp);
        తిరిగి resp;
    }();

    // Assemble the body
    funcp->కూడు_Stmtsp(stmtsp);

    //పూర్తి
    తిరిగి funcp;
}
