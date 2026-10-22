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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderGraph.h"
#अन्तर्भाव "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

वर्ग V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    ध्रुव string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ध्रुव V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    शून्य processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        प्रति (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* ध्रुव vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            यदि (vtxp->domainp()) अनुवर्त;

            // Reset state
            triggers.clear();

            OrderLogicVertex* ध्रुव lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            यदि (ध्रुव OrderDomain* ध्रुव hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                ध्रुव std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.घाला(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            प्रति (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* ध्रुव fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                यदि (!edge.weight()) अनुवर्त;
                // some edges can be ignored as they only encode ordering constraints
                यदि (!fromVtxp->domainMatters()) अनुवर्त;

                // Add driver's domains
                ध्रुव OrderDomain* ध्रुव fromDomainp = fromVtxp->domainp();
                ध्रुव std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.घाला(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                यदि (OrderVarVertex* ध्रुव varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ध्रुव OrderDomain* ध्रुव domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            यदि (domainp->triggers().empty() && lvtxp) m_untriggeredps.ढकला_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, ध्रुव string& tag,
                          ध्रुव V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        यदि (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        प्रति (OrderLogicVertex* ध्रुव lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_वृक्ष();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = स्वभाव;

सार्वजनिक:
    // Order the logic
    स्थिर शून्य apply(OrderGraph& graph, ध्रुव string& tag,
                      ध्रुव V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

शून्य V3Order::processDomains(OrderGraph& graph,  //
                             ध्रुव std::string& tag,  //
                             ध्रुव ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
