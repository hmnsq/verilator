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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3OrderGraph.h"
#அந்தர்பாவ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

வகுப்பு V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    மாறிலி string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    மாறிலி V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    வெறுமை processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        க்கு (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* மாறிலி vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            எனில் (vtxp->domainp()) தொடர்;

            // Reset state
            triggers.clear();

            OrderLogicVertex* மாறிலி lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            எனில் (மாறிலி OrderDomain* மாறிலி hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                மாறிலி std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.செருகு(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            க்கு (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* மாறிலி fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                எனில் (!edge.weight()) தொடர்;
                // some edges can be ignored as they only encode ordering constraints
                எனில் (!fromVtxp->domainMatters()) தொடர்;

                // Add driver's domains
                மாறிலி OrderDomain* மாறிலி fromDomainp = fromVtxp->domainp();
                மாறிலி std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.செருகு(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                எனில் (OrderVarVertex* மாறிலி varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            மாறிலி OrderDomain* மாறிலி domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            எனில் (domainp->triggers().empty() && lvtxp) m_untriggeredps.தள்ளு_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, மாறிலி string& tag,
                          மாறிலி V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        எனில் (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        க்கு (OrderLogicVertex* மாறிலி lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_மரம்();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = இயல்பு;

பொது:
    // Order the logic
    நிலையான வெறுமை apply(OrderGraph& graph, மாறிலி string& tag,
                      மாறிலி V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

வெறுமை V3Order::processDomains(OrderGraph& graph,  //
                             மாறிலி std::string& tag,  //
                             மாறிலி ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
