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
    स्थिरांक string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    स्थिरांक V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    शून्य processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        साठी (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* स्थिरांक vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            जर (vtxp->domainp()) सुरूठेवा;

            // Reset state
            triggers.clear();

            OrderLogicVertex* स्थिरांक lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            जर (स्थिरांक OrderDomain* स्थिरांक hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                स्थिरांक std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.घाला(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            साठी (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* स्थिरांक fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                जर (!edge.weight()) सुरूठेवा;
                // some edges can be ignored as they only encode ordering constraints
                जर (!fromVtxp->domainMatters()) सुरूठेवा;

                // Add driver's domains
                स्थिरांक OrderDomain* स्थिरांक fromDomainp = fromVtxp->domainp();
                स्थिरांक std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.घाला(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                जर (OrderVarVertex* स्थिरांक varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            स्थिरांक OrderDomain* स्थिरांक domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            जर (domainp->triggers().empty() && lvtxp) m_untriggeredps.ढकला_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, स्थिरांक string& tag,
                          स्थिरांक V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        जर (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        साठी (OrderLogicVertex* स्थिरांक lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_वृक्ष();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = पूर्वनिर्धारित;

सार्वजनिक:
    // Order the logic
    स्थिर शून्य apply(OrderGraph& graph, स्थिरांक string& tag,
                      स्थिरांक V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

शून्य V3Order::processDomains(OrderGraph& graph,  //
                             स्थिरांक std::string& tag,  //
                             स्थिरांक ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
