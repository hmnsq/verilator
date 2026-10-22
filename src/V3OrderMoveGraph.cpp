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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3OrderMoveGraph.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 ଅଚଳ OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

ଶ୍ରେଣୀ OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<ଅଚଳ OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{ନୂତନ OrderMoveGraph{m_orderGraph}};
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
        ପାଇଁ (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            ଯଦି (OrderLogicVertex* ଅଚଳ lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(ନୂତନ OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } ଅନ୍ୟଥା {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        ପାଇଁ (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            ଯଦି (OrderLogicVertex* ଅଚଳ lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    ଆଭାସୀ ~OrderMoveGraphBuilder() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    ତାର୍କିକ domainsExclusive(ଅଚଳ OrderDomain* fromp, ଅଚଳ OrderDomain* ଶୀର୍ଷ) {
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

        ଅଚଳ std::set<uint32_t>& fromTriggers = fromp->triggers();
        ଅଚଳ std::set<uint32_t>& toTriggers = ଶୀର୍ଷ->triggers();
        ଯଦି (fromTriggers.size() != 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (toTriggers.size() != 1) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstSenItem* ଅଚଳ fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        ଯଦି (!fromSenItemp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstSenItem* ଅଚଳ toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        ଯଦି (!toSenItemp) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ fromVarrefp = fromSenItemp->varrefp();
        ଯଦି (!fromVarrefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ toVarrefp = toSenItemp->varrefp();
        ଯଦି (!toVarrefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        ଯଦି (fromVarrefp->varScopep() != toVarrefp->varScopep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ଫେରନ୍ତୁ fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    ଶୂନ୍ୟ addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        ନୂତନ V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    ଶୂନ୍ୟ iterateLogicVertex(ଅଚଳ OrderLogicVertex* lvtxp) {
        ଅଚଳ OrderDomain* ଅଚଳ domainp = lvtxp->domainp();
        OrderMoveVertex* ଅଚଳ lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : lvtxp->outEdges()) {
            ଯଦି (edge.weight() == 0) ଜାରି;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ଅଚଳ OrderVarVertex* ଅଚଳ vvtxp = static_cast<ଅଚଳ OrderVarVertex*>(edge.ଶୀର୍ଷ());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ଅଚଳ auto pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            ଯଦି (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            ଯଦି (!vMoveVtxp) ଜାରି;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(ଅଚଳ OrderVarVertex* vvtxp, ଅଚଳ OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vvtxp->outEdges()) {
            ଯଦି (edge.weight() == 0) ଜାରି;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ଅଚଳ OrderLogicVertex* ଅଚଳ lVtxp = edge.ଶୀର୍ଷ()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            ଯଦି (domainsExclusive(domainp, lVtxp->domainp())) ଜାରି;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            ଯଦି (!vMoveVtxp) vMoveVtxp = ନୂତନ OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* ଅଚଳ lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        ଫେରନ୍ତୁ vMoveVtxp;
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        ଫେରନ୍ତୁ std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    ଫେରନ୍ତୁ OrderMoveGraphBuilder::apply(orderGraph);
}
