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
//  OrderMoveGraph implementation and related
//
//*************************************************************************

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3OrderMoveGraph.h"

#அந்தர்பாவ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 மாறிலி OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

வகுப்பு OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<மாறிலி OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{புதிய OrderMoveGraph{m_orderGraph}};
    // Storage for domain -> OrderMoveVertex, maps held in OrderVarVertex::userp()
    std::deque<DomainMap> m_domainMaps;

    // CONSTRUCTORS
    explicit OrderMoveGraphBuilder(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {
        // How this works:
        //  - Create a OrderMoveVertex for each OrderLogicVertex.
        //  - Following each OrderLogicVertex, search forward in the context of its domain
        //    - If we encounter another OrderLogicVertex in non-exclusive domain, make the
        //      OrderMoveVertex->OrderMoveVertex edge.
        //    - If we encounter an OrderVarVertex, make a Vertex for the (OrderVarVertex, domain)
        //      pair and continue to search forward in the context of the same domain.  Unless we
        //      already created that pair, in which case, we've already done the forward search,
        //      so stop.

        // For each logic vertex, make a OrderMoveVertex, for each variable vertex, allocate
        // storage
        க்கு (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            எனில் (OrderLogicVertex* மாறிலி lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(புதிய OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } இல்லையெனில் {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        க்கு (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            எனில் (OrderLogicVertex* மாறிலி lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    மெய்நிகர் ~OrderMoveGraphBuilder() = இயல்பு;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    தருக்க domainsExclusive(மாறிலி OrderDomain* fromp, மாறிலி OrderDomain* உச்சி) {
        // Return 'true' if we can prove that both 'from' and 'to' cannot both
        // be active on the same evaluation, or false if we can't prove this.
        //
        // This detects the case of 'always @(posedge clk)'
        // and 'always @(negedge clk)' being exclusive.
        //
        // Are there any other cases we need to handle? Maybe not,
        // because these are not exclusive:
        //   always @(posedge A or posedge B)
        //   always @(negedge A)
        //
        // ... unless you know more about A and B, which sounds hard.

        மாறிலி std::set<uint32_t>& fromTriggers = fromp->triggers();
        மாறிலி std::set<uint32_t>& toTriggers = உச்சி->triggers();
        எனில் (fromTriggers.size() != 1) திரும்பு பொய்;
        எனில் (toTriggers.size() != 1) திரும்பு பொய்;
        மாறிலி AstSenItem* மாறிலி fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        எனில் (!fromSenItemp) திரும்பு பொய்;
        மாறிலி AstSenItem* மாறிலி toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        எனில் (!toSenItemp) திரும்பு பொய்;

        மாறிலி Ast_முனை_Var_Ref* மாறிலி fromVarrefp = fromSenItemp->varrefp();
        எனில் (!fromVarrefp) திரும்பு பொய்;
        மாறிலி Ast_முனை_Var_Ref* மாறிலி toVarrefp = toSenItemp->varrefp();
        எனில் (!toVarrefp) திரும்பு பொய்;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        எனில் (fromVarrefp->varScopep() != toVarrefp->varScopep()) திரும்பு பொய்;

        திரும்பு fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    வெறுமை addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        புதிய V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    வெறுமை iterateLogicVertex(மாறிலி OrderLogicVertex* lvtxp) {
        மாறிலி OrderDomain* மாறிலி domainp = lvtxp->domainp();
        OrderMoveVertex* மாறிலி lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        க்கு (மாறிலி V3GraphEdge& edge : lvtxp->outEdges()) {
            எனில் (edge.weight() == 0) தொடர்;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            மாறிலி OrderVarVertex* மாறிலி vvtxp = static_cast<மாறிலி OrderVarVertex*>(edge.உச்சி());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            மாறிலி auto pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            எனில் (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            எனில் (!vMoveVtxp) தொடர்;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(மாறிலி OrderVarVertex* vvtxp, மாறிலி OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        க்கு (மாறிலி V3GraphEdge& edge : vvtxp->outEdges()) {
            எனில் (edge.weight() == 0) தொடர்;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            மாறிலி OrderLogicVertex* மாறிலி lVtxp = edge.உச்சி()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            எனில் (domainsExclusive(domainp, lVtxp->domainp())) தொடர்;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            எனில் (!vMoveVtxp) vMoveVtxp = புதிய OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* மாறிலி lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        திரும்பு vMoveVtxp;
    }

பொது:
    நிலையான std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        திரும்பு std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    திரும்பு OrderMoveGraphBuilder::apply(orderGraph);
}
