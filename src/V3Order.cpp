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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ಶೂನ್ಯ V3_ಕ್ರಮ::ಕ್ರಮ_ಕ್ರಮ_ನಕ್ಷೆ(ಕ್ರಮ_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್) {
    // Dump data
    ಆದರೆ (dump_ನಕ್ಷೆ_Level()) ನಕ್ಷೆ.dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_orderg_pre");

    // Break cycles. Note that the OrderGraph only contains cuttable cycles
    // (soft constraints). Actual logic loops must have been eliminated by
    // the introduction of Hybid sensitivity expressions, before invoking
    // ordering (e.g. in V3SchedAcyclic).
    ನಕ್ಷೆ.acyclic(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level()) ನಕ್ಷೆ.dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_orderg_acyc");

    // Assign ranks so we know what to follow, then sort vertices and edges by that ordering
    ನಕ್ಷೆ.ಕ್ರಮ();
    ಆದರೆ (dump_ನಕ್ಷೆ_Level()) ನಕ್ಷೆ.dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_orderg_order");
}

//######################################################################

AstCFunc* V3_ಕ್ರಮ::ಕ್ರಮ(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& logic,  //
                         ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit,  //
                         AstVarScope* trigVscp,  //
                         ಸ್ಥಿರಾಂಕ V3Sched::CovergroupRefBindings& cgRefBindings,
                         ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್,  //
                         ತಾರ್ಕಿಕ parallel,  //
                         ತಾರ್ಕಿಕ slow,  //
                         ಸ್ಥಿರಾಂಕ ExternalTriggersProvider& externalTriggers) {
    // Build the OrderGraph
    ಸ್ಥಿರಾಂಕ std::unique_ptr<ಕ್ರಮ_ನಕ್ಷೆ> ನಕ್ಷೆ
        = build_ಕ್ರಮ_ನಕ್ಷೆ(trigKit, trigVscp, logic, cgRefBindings, parallel);
    // Order it
    ಕ್ರಮ_ಕ್ರಮ_ನಕ್ಷೆ(*ನಕ್ಷೆ, ಟ್ಯಾಗ್);
    // Assign sensitivity domains to combinational logic
    processDomains(*ನಕ್ಷೆ, ಟ್ಯಾಗ್, externalTriggers);
    // Build the move graph
    ಕ್ರಮ_Move_Dom_Scope::clear();
    ಸ್ಥಿರಾಂಕ std::unique_ptr<ಕ್ರಮ_Move_ನಕ್ಷೆ> moveGraphp = ಕ್ರಮ_Move_ನಕ್ಷೆ::build(*ನಕ್ಷೆ);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) moveGraphp->dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_ordermv");

    // The ordered statements, if there are any
    Ast_ಗಂಟು_Stmt* stmtsp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (!moveGraphp->ಖಾಲಿಗಳು()) {
        ಆದರೆ (parallel) {
            stmtsp = createParallel(*moveGraphp, ಟ್ಯಾಗ್, slow);
        } ಇಲ್ಲದಿದ್ದರೆ {
            stmtsp = createSerial(*moveGraphp, ಟ್ಯಾಗ್, slow);
        }
        // Should have consumed all vertices
        UASSERT(moveGraphp->ಖಾಲಿಗಳು(), "Unconsumed vertices remain in OrderMoveGraph");
    }
    ಕ್ರಮ_Move_Dom_Scope::clear();

    // Dump data
    ಆದರೆ (dump_ನಕ್ಷೆ_Level()) ನಕ್ಷೆ->dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_orderg_done");

    // Dispose of the remnants of the inputs
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ lbsp : logic) lbsp->deleteActives();

    // If there is no resulting logic, then don't create an empty function
    ಆದರೆ (!stmtsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

    // Create the result function
    AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->fileline();
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = [&]() {
        AstScope* ಸ್ಥಿರಾಂಕ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
        AstCFunc* ಸ್ಥಿರಾಂಕ resp = ಹೊಸ AstCFunc{flp, "_eval_body__" + ಟ್ಯಾಗ್, scopeTopp, ""};
        resp->dontCombine(ನಿಜ);
        resp->isStatic(ಸುಳ್ಳು);
        resp->isLoose(ನಿಜ);
        resp->slow(slow);
        resp->isConst(ಸುಳ್ಳು);
        resp->decl_ಖಾಸಗಿ(ನಿಜ);
        scopeTopp->ಕೂಡಿಸು_Blocksp(resp);
        ಹಿಂತಿರುಗಿ resp;
    }();

    // Assemble the body
    funcp->ಕೂಡಿಸು_Stmtsp(stmtsp);

    //ಮುಗಿದ
    ಹಿಂತಿರುಗಿ funcp;
}
