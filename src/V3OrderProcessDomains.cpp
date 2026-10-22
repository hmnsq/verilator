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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3OrderGraph.h"
#समावेश "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

श्रेणी V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    स्थिर string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    स्थिर V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    व्योम processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        क्रम (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* स्थिर vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            अगर (vtxp->domainp()) जारी;

            // Reset state
            triggers.सफा();

            OrderLogicVertex* स्थिर lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            अगर (स्थिर OrderDomain* स्थिर hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                स्थिर std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.जोड़ो(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            क्रम (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* स्थिर fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                अगर (!edge.weight()) जारी;
                // some edges can be ignored as they only encode ordering constraints
                अगर (!fromVtxp->domainMatters()) जारी;

                // Add driver's domains
                स्थिर OrderDomain* स्थिर fromDomainp = fromVtxp->domainp();
                स्थिर std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.जोड़ो(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                अगर (OrderVarVertex* स्थिर varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            स्थिर OrderDomain* स्थिर domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            अगर (domainp->triggers().empty() && lvtxp) m_untriggeredps.धकेलो_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, स्थिर string& tag,
                          स्थिर V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        अगर (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        क्रम (OrderLogicVertex* स्थिर lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_वृक्ष();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = शेष;

खुला:
    // Order the logic
    अटल व्योम apply(OrderGraph& graph, स्थिर string& tag,
                      स्थिर V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

व्योम V3Order::processDomains(OrderGraph& graph,  //
                             स्थिर std::string& tag,  //
                             स्थिर ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
