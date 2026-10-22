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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderMoveGraph.h"

#अन्तर्भाव "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 स्थिरांक OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

वर्ग OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<स्थिरांक OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{नवीन OrderMoveGraph{m_orderGraph}};
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
        साठी (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            जर (OrderLogicVertex* स्थिरांक lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(नवीन OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } अन्यथा {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        साठी (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            जर (OrderLogicVertex* स्थिरांक lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    आभासी ~OrderMoveGraphBuilder() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    तार्किक domainsExclusive(स्थिरांक OrderDomain* fromp, स्थिरांक OrderDomain* शीर्ष) {
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

        स्थिरांक std::set<uint32_t>& fromTriggers = fromp->triggers();
        स्थिरांक std::set<uint32_t>& toTriggers = शीर्ष->triggers();
        जर (fromTriggers.size() != 1) परत असत्य;
        जर (toTriggers.size() != 1) परत असत्य;
        स्थिरांक AstSenItem* स्थिरांक fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        जर (!fromSenItemp) परत असत्य;
        स्थिरांक AstSenItem* स्थिरांक toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        जर (!toSenItemp) परत असत्य;

        स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक fromVarrefp = fromSenItemp->varrefp();
        जर (!fromVarrefp) परत असत्य;
        स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक toVarrefp = toSenItemp->varrefp();
        जर (!toVarrefp) परत असत्य;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        जर (fromVarrefp->varScopep() != toVarrefp->varScopep()) परत असत्य;

        परत fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    शून्य addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        नवीन V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    शून्य iterateLogicVertex(स्थिरांक OrderLogicVertex* lvtxp) {
        स्थिरांक OrderDomain* स्थिरांक domainp = lvtxp->domainp();
        OrderMoveVertex* स्थिरांक lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        साठी (स्थिरांक V3GraphEdge& edge : lvtxp->outEdges()) {
            जर (edge.weight() == 0) सुरूठेवा;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            स्थिरांक OrderVarVertex* स्थिरांक vvtxp = static_cast<स्थिरांक OrderVarVertex*>(edge.शीर्ष());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            स्थिरांक स्वतः pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            जर (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            जर (!vMoveVtxp) सुरूठेवा;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(स्थिरांक OrderVarVertex* vvtxp, स्थिरांक OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        साठी (स्थिरांक V3GraphEdge& edge : vvtxp->outEdges()) {
            जर (edge.weight() == 0) सुरूठेवा;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            स्थिरांक OrderLogicVertex* स्थिरांक lVtxp = edge.शीर्ष()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            जर (domainsExclusive(domainp, lVtxp->domainp())) सुरूठेवा;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            जर (!vMoveVtxp) vMoveVtxp = नवीन OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* स्थिरांक lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        परत vMoveVtxp;
    }

सार्वजनिक:
    स्थिर std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        परत std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    परत OrderMoveGraphBuilder::apply(orderGraph);
}
