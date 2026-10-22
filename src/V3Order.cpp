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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderInternal.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਖਾਲੀ V3_ਕ੍ਰਮ::ਕ੍ਰਮ_ਕ੍ਰਮ_ਗ੍ਰਾਫ(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ std::string& ਟੈਗ) {
    // Dump data
    ਜੇ (dump_ਗ੍ਰਾਫ_Level()) ਗ੍ਰਾਫ.dumpDotFilePrefixed(ਟੈਗ + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    ਗ੍ਰਾਫ.acyclic(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level()) ਗ੍ਰਾਫ.dumpDotFilePrefixed(ਟੈਗ + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    ਗ੍ਰਾਫ.ਕ੍ਰਮ();
    ਜੇ (dump_ਗ੍ਰਾਫ_Level()) ਗ੍ਰਾਫ.dumpDotFilePrefixed(ਟੈਗ + "_orderg_order");
}

//######################################################################

AstCFunc* V3_ਕ੍ਰਮ::ਕ੍ਰਮ(ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& logic,  //
                         ਅਚਲ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ਅਚਲ V3Sched::CovergroupRefBindings& cgRefBindings,
                         ਅਚਲ string& ਟੈਗ,  //
                         ਤਾਰਕਿਕ parallel,  //
                         ਤਾਰਕਿਕ slow,  //
                         ਅਚਲ ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    ਅਚਲ std::unique_ptr<ਕ੍ਰਮ_ਗ੍ਰਾਫ> ਗ੍ਰਾਫ
        = build_ਕ੍ਰਮ_ਗ੍ਰਾਫ(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    ਕ੍ਰਮ_ਕ੍ਰਮ_ਗ੍ਰਾਫ(*ਗ੍ਰਾਫ, ਟੈਗ);
    // Assign sensitivity domains to combinational logic
    processDomains(*ਗ੍ਰਾਫ, ਟੈਗ, externalTriggers);
    // Build the move graph
    ਕ੍ਰਮ_Move_Dom_Scope::clear();
    ਅਚਲ std::unique_ptr<ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ> moveGraphp = ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ::build(*ਗ੍ਰਾਫ);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) moveGraphp->dumpDotFilePrefixed(ਟੈਗ + "_ordermv");

    // The ordered statements, if there are any
    Ast_ਗੰਢ_Stmt* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (!moveGraphp->ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਜੇ (parallel) {
            stmtsp = createParallel(*moveGraphp, ਟੈਗ, slow);
        } ਨਹੀਂਤਾਂ {
            stmtsp = createSerial(*moveGraphp, ਟੈਗ, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->ਖਾਲੀ_ਥਾਵਾਂ(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    ਕ੍ਰਮ_Move_Dom_Scope::clear();

    // Dump data
    ਜੇ (dump_ਗ੍ਰਾਫ_Level()) ਗ੍ਰਾਫ->dumpDotFilePrefixed(ਟੈਗ + "_orderg_done");

    // Dispose of the remnants of the inputs
    ਲਈ (ਸਵੈਚਾਲਿਤ* ਅਚਲ lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    ਜੇ (!stmtsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

    // Create the result function
    AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->fileline();
    AstCFunc* ਅਚਲ funcp = [&]() {
        AstScope* ਅਚਲ scopeTopp = netlistp->ਸਿਖਰ_Scopep()->scopep();
        AstCFunc* ਅਚਲ resp = ਨਵਾਂ AstCFunc{flp, "_eval_body__" + ਟੈਗ, scopeTopp, ""};
        resp->dontCombine(ਸੱਚ);
        resp->isStatic(ਝੂਠ);
        resp->isLoose(ਸੱਚ);
        resp->slow(slow);
        resp->isConst(ਝੂਠ);
        resp->decl_ਨਿੱਜੀ(ਸੱਚ);
        scopeTopp->ਜੋੜੋ_Blocksp(resp);
        ਵਾਪਸ resp;
    }();

    // Assemble the body
    funcp->ਜੋੜੋ_Stmtsp(stmtsp);

    //ਪੂਰਾ
    ਵਾਪਸ funcp;
}
