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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderMoveGraph.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 ಧ್ರುವ OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

ವರ್ಗ OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<ಧ್ರುವ OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{ನೂತನ OrderMoveGraph{m_orderGraph}};
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
        ಪ್ರತಿ (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            ಯದಿ (OrderLogicVertex* ಧ್ರುವ lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(ನೂತನ OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } ಅನ್ಯಥಾ {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        ಪ್ರತಿ (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            ಯದಿ (OrderLogicVertex* ಧ್ರುವ lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    ಆಭಾಸ ~OrderMoveGraphBuilder() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    ತಾರ್ಕಿಕ domainsExclusive(ಧ್ರುವ OrderDomain* fromp, ಧ್ರುವ OrderDomain* ತುದಿ) {
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

        ಧ್ರುವ std::set<uint32_t>& fromTriggers = fromp->triggers();
        ಧ್ರುವ std::set<uint32_t>& toTriggers = ತುದಿ->triggers();
        ಯದಿ (fromTriggers.size() != 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (toTriggers.size() != 1) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstSenItem* ಧ್ರುವ fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        ಯದಿ (!fromSenItemp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstSenItem* ಧ್ರುವ toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        ಯದಿ (!toSenItemp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ fromVarrefp = fromSenItemp->varrefp();
        ಯದಿ (!fromVarrefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ toVarrefp = toSenItemp->varrefp();
        ಯದಿ (!toVarrefp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        ಯದಿ (fromVarrefp->varScopep() != toVarrefp->varScopep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಪ್ರತ್ಯಾಗಮ fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    ಶೂನ್ಯ addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        ನೂತನ V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    ಶೂನ್ಯ iterateLogicVertex(ಧ್ರುವ OrderLogicVertex* lvtxp) {
        ಧ್ರುವ OrderDomain* ಧ್ರುವ domainp = lvtxp->domainp();
        OrderMoveVertex* ಧ್ರುವ lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : lvtxp->outEdges()) {
            ಯದಿ (edge.weight() == 0) ಅನುವರ್ತ;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ಧ್ರುವ OrderVarVertex* ಧ್ರುವ vvtxp = static_cast<ಧ್ರುವ OrderVarVertex*>(edge.ತುದಿ());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ಧ್ರುವ auto pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            ಯದಿ (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            ಯದಿ (!vMoveVtxp) ಅನುವರ್ತ;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(ಧ್ರುವ OrderVarVertex* vvtxp, ಧ್ರುವ OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vvtxp->outEdges()) {
            ಯದಿ (edge.weight() == 0) ಅನುವರ್ತ;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ಧ್ರುವ OrderLogicVertex* ಧ್ರುವ lVtxp = edge.ತುದಿ()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            ಯದಿ (domainsExclusive(domainp, lVtxp->domainp())) ಅನುವರ್ತ;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            ಯದಿ (!vMoveVtxp) vMoveVtxp = ನೂತನ OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* ಧ್ರುವ lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        ಪ್ರತ್ಯಾಗಮ vMoveVtxp;
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        ಪ್ರತ್ಯಾಗಮ std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    ಪ್ರತ್ಯಾಗಮ OrderMoveGraphBuilder::apply(orderGraph);
}
