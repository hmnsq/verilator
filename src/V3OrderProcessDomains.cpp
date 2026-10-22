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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3OrderGraph.h"
#অন্তর্ভাব "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

শ্রেণী V3OrderProcessDomains final {
    // STATE
    OrderGraph& m_graph;  // The ordering graph
    ধ্রুবক string m_tag;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ধ্রুবক V3Order::ExternalTriggersProvider m_externalTriggers;
    std::vector<OrderLogicVertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    শূন্য processDomains() {
        // The triggers of the current vertex
        std::set<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        জন্য (V3GraphVertex& it : m_graph.vertices()) {
            OrderEitherVertex* ধ্রুবক vtxp = it.as<OrderEitherVertex>();
            // Sequential logic already has its domain set
            যদি (vtxp->domainp()) চালু;

            // Reset state
            triggers.clear();

            OrderLogicVertex* ধ্রুবক lvtxp = vtxp->cast<OrderLogicVertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            যদি (ধ্রুবক OrderDomain* ধ্রুবক hybridp = lvtxp ? lvtxp->hybridp() : nullptr) {
                ধ্রুবক std::set<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.ঢোকাও(hybridTriggers.begin(), hybridTriggers.end());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            জন্য (V3GraphEdge& edge : vtxp->inEdges()) {
                OrderEitherVertex* ধ্রুবক fromVtxp = edge.fromp()->as<OrderEitherVertex>();
                // Cut edge
                যদি (!edge.weight()) চালু;
                // some edges can be ignored as they only encode ordering constraints
                যদি (!fromVtxp->domainMatters()) চালু;

                // Add driver's domains
                ধ্রুবক OrderDomain* ধ্রুবক fromDomainp = fromVtxp->domainp();
                ধ্রুবক std::set<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.ঢোকাও(fromTriggers.begin(), fromTriggers.end());

                // Add in any external domains of variables
                যদি (OrderVarVertex* ধ্রুবক varVtxp = fromVtxp->cast<OrderVarVertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ধ্রুবক OrderDomain* ধ্রুবক domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            যদি (domainp->triggers().empty() && lvtxp) m_untriggeredps.ঠেলো_back(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3OrderProcessDomains(OrderGraph& graph, ধ্রুবক string& tag,
                          ধ্রুবক V3Order::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_tag{tag}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        যদি (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_tag + "_orderg_domain");

        // Delete logic that is never triggered
        জন্য (OrderLogicVertex* ধ্রুবক lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().empty(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlinkFrBack()->delete_বৃক্ষ();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3OrderProcessDomains() = পূর্বনির্ধারিত;

সর্বজনীন:
    // Order the logic
    স্থির শূন্য apply(OrderGraph& graph, ধ্রুবক string& tag,
                      ধ্রুবক V3Order::ExternalTriggersProvider& externalTriggers) {
        V3OrderProcessDomains{graph, tag, externalTriggers};
    }
};

শূন্য V3Order::processDomains(OrderGraph& graph,  //
                             ধ্রুবক std::string& tag,  //
                             ধ্রুবক ExternalTriggersProvider& externalTriggers) {
    V3OrderProcessDomains::apply(graph, tag, externalTriggers);
}
