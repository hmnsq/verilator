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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3OrderGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

ଶ୍ରେଣୀ V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    ଅଚଳ string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ଅଚଳ V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    ଶୂନ୍ୟ processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        ପାଇଁ (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* ଅଚଳ vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            ଯଦି (vtxp->domainp()) ଜାରି;

            // Reset state
            triggers.clear();

            OrderLogicVertex* ଅଚଳ lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            ଯଦି (ଅଚଳ OrderDomain* ଅଚଳ hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                ଅଚଳ std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.ଭର୍ତ୍ତି(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            ପାଇଁ (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* ଅଚଳ fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                ଯଦି (!edge.weight()) ଜାରି;
                // some edges can be ignored as they only encode ordering constraints
                ଯଦି (!fromVtxp->domainMatters()) ଜାରି;

                // Add driver's domains
                ଅଚଳ OrderDomain* ଅଚଳ fromDomainp = fromVtxp->domainp();
                ଅଚଳ std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.ଭର୍ତ୍ତି(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                ଯଦି (OrderVarVertex* ଅଚଳ varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ଅଚଳ OrderDomain* ଅଚଳ domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            ଯଦି (domainp->triggers().empty() && lvtxp) m_untriggeredps.ଠେଲ_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, ଅଚଳ string& tag,
                          ଅଚଳ V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        ଯଦି (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        ପାଇଁ (OrderLogicVertex* ଅଚଳ lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    // Order the logic
    ସ୍ଥିର ଶୂନ୍ୟ apply(OrderGraph& graph, ଅଚଳ string& tag,
                      ଅଚଳ V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

ଶୂନ୍ୟ V3Order::processDomains(OrderGraph& graph,  //
                             ଅଚଳ std::string& tag,  //
                             ଅଚଳ ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
