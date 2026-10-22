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
//  Initial graph dependency builder for ordering
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3OrderGraph.h"
#અન્તર્ભાવ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

વર્ગ V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    અચલ string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    અચલ V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    શૂન્ય processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        માટે (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* અચલ vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            જો (vtxp->domainp()) ચાલુ;

            // Reset state
            triggers.clear();

            OrderLogicVertex* અચલ lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            જો (અચલ OrderDomain* અચલ hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                અચલ std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.ઉમેરો(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            માટે (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* અચલ fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                જો (!edge.weight()) ચાલુ;
                // some edges can be ignored as they only encode ordering constraints
                જો (!fromVtxp->domainMatters()) ચાલુ;

                // Add driver's domains
                અચલ OrderDomain* અચલ fromDomainp = fromVtxp->domainp();
                અચલ std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.ઉમેરો(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                જો (OrderVarVertex* અચલ varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            અચલ OrderDomain* અચલ domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            જો (domainp->triggers().empty() && lvtxp) m_untriggeredps.ધકેલો_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, અચલ string& tag,
                          અચલ V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        જો (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        માટે (OrderLogicVertex* અચલ lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_વૃક્ષ();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = મૂળભૂત;

જાહેર:
    // Order the logic
    સ્થિર શૂન્ય apply(OrderGraph& graph, અચલ string& tag,
                      અચલ V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

શૂન્ય V3Order::processDomains(OrderGraph& graph,  //
                             અચલ std::string& tag,  //
                             અચલ ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
