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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3OrderGraph.h"
#അന്തര്ഭാവ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

വർഗ്ഗം V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    സ്ഥിരം string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    സ്ഥിരം V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    ശൂന്യം processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        വേണ്ടി (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* സ്ഥിരം vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            എങ്കിൽ (vtxp->domainp()) തുടരുക;

            // Reset state
            triggers.clear();

            OrderLogicVertex* സ്ഥിരം lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            എങ്കിൽ (സ്ഥിരം OrderDomain* സ്ഥിരം hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                സ്ഥിരം std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.തിരുകുക(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            വേണ്ടി (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* സ്ഥിരം fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                എങ്കിൽ (!edge.weight()) തുടരുക;
                // some edges can be ignored as they only encode ordering constraints
                എങ്കിൽ (!fromVtxp->domainMatters()) തുടരുക;

                // Add driver's domains
                സ്ഥിരം OrderDomain* സ്ഥിരം fromDomainp = fromVtxp->domainp();
                സ്ഥിരം std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.തിരുകുക(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                എങ്കിൽ (OrderVarVertex* സ്ഥിരം varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            സ്ഥിരം OrderDomain* സ്ഥിരം domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            എങ്കിൽ (domainp->triggers().empty() && lvtxp) m_untriggeredps.തള്ളുക_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, സ്ഥിരം string& tag,
                          സ്ഥിരം V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        എങ്കിൽ (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        വേണ്ടി (OrderLogicVertex* സ്ഥിരം lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_വൃക്ഷം();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = സ്ഥിരസ്ഥിതി;

പൊതു:
    // Order the logic
    സ്ഥിതം ശൂന്യം apply(OrderGraph& graph, സ്ഥിരം string& tag,
                      സ്ഥിരം V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

ശൂന്യം V3Order::processDomains(OrderGraph& graph,  //
                             സ്ഥിരം std::string& tag,  //
                             സ്ഥിരം ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
