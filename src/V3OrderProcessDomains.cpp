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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

ವರ್ಗ V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    ಧ್ರುವ string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ಧ್ರುವ V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    ಶೂನ್ಯ processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        ಪ್ರತಿ (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* ಧ್ರುವ vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            ಯದಿ (vtxp->domainp()) ಅನುವರ್ತ;

            // Reset state
            triggers.clear();

            OrderLogicVertex* ಧ್ರುವ lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            ಯದಿ (ಧ್ರುವ OrderDomain* ಧ್ರುವ hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                ಧ್ರುವ std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.ಸೇರಿಸು(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            ಪ್ರತಿ (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* ಧ್ರುವ fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                ಯದಿ (!edge.weight()) ಅನುವರ್ತ;
                // some edges can be ignored as they only encode ordering constraints
                ಯದಿ (!fromVtxp->domainMatters()) ಅನುವರ್ತ;

                // Add driver's domains
                ಧ್ರುವ OrderDomain* ಧ್ರುವ fromDomainp = fromVtxp->domainp();
                ಧ್ರುವ std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.ಸೇರಿಸು(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                ಯದಿ (OrderVarVertex* ಧ್ರುವ varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ಧ್ರುವ OrderDomain* ಧ್ರುವ domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            ಯದಿ (domainp->triggers().empty() && lvtxp) m_untriggeredps.ತಳ್ಳು_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, ಧ್ರುವ string& tag,
                          ಧ್ರುವ V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        ಯದಿ (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        ಪ್ರತಿ (OrderLogicVertex* ಧ್ರುವ lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    // Order the logic
    ಸ್ಥಿರ ಶೂನ್ಯ apply(OrderGraph& graph, ಧ್ರುವ string& tag,
                      ಧ್ರುವ V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

ಶೂನ್ಯ V3Order::processDomains(OrderGraph& graph,  //
                             ಧ್ರುವ std::string& tag,  //
                             ಧ್ರುವ ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
