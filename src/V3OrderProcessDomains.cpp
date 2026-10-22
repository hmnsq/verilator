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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderGraph.h"
#ضمن "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

صنف V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    ثابت string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ثابت V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    فراغ processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        لكل (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* ثابت vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            𐡄𐡍 (vtxp->domainp()) استمر;

            // Reset state
            triggers.clear();

            OrderLogicVertex* ثابت lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            𐡄𐡍 (ثابت OrderDomain* ثابت hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                ثابت std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.insert(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            لكل (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* ثابت fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                𐡄𐡍 (!edge.weight()) استمر;
                // some edges can be ignored as they only encode ordering constraints
                𐡄𐡍 (!fromVtxp->domainMatters()) استمر;

                // Add driver's domains
                ثابت OrderDomain* ثابت fromDomainp = fromVtxp->domainp();
                ثابت std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.insert(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                𐡄𐡍 (OrderVarVertex* ثابت varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ثابت OrderDomain* ثابت domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            𐡄𐡍 (domainp->triggers().empty() && lvtxp) m_untriggeredps.push_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, ثابت string& tag,
                          ثابت V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        𐡄𐡍 (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        لكل (OrderLogicVertex* ثابت lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->deleteTree();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = افتراضي;

عام:
    // Order the logic
    ساكن فراغ apply(OrderGraph& graph, ثابت string& tag,
                      ثابت V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

فراغ V3Order::processDomains(OrderGraph& graph,  //
                             ثابت std::string& tag,  //
                             ثابت ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
