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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3OrderMoveGraph.h"

#অন্তর্ভাৱ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 ধ্রুৱ OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

শ্ৰেণী OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<ধ্রুৱ OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{নূতন OrderMoveGraph{m_orderGraph}};
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
        বাবে (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            যদি (OrderLogicVertex* ধ্রুৱ lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(নূতন OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } অন্যথা {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        বাবে (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            যদি (OrderLogicVertex* ধ্রুৱ lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    আভাস ~OrderMoveGraphBuilder() = স্ৱভাৱ;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    তার্কিক domainsExclusive(ধ্রুৱ OrderDomain* fromp, ধ্রুৱ OrderDomain* শীৰ্ষ) {
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

        ধ্রুৱ std::set<uint32_t>& fromTriggers = fromp->triggers();
        ধ্রুৱ std::set<uint32_t>& toTriggers = শীৰ্ষ->triggers();
        যদি (fromTriggers.size() != 1) ঘূৰাই মিছা;
        যদি (toTriggers.size() != 1) ঘূৰাই মিছা;
        ধ্রুৱ AstSenItem* ধ্রুৱ fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        যদি (!fromSenItemp) ঘূৰাই মিছা;
        ধ্রুৱ AstSenItem* ধ্রুৱ toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        যদি (!toSenItemp) ঘূৰাই মিছা;

        ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ fromVarrefp = fromSenItemp->varrefp();
        যদি (!fromVarrefp) ঘূৰাই মিছা;
        ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ toVarrefp = toSenItemp->varrefp();
        যদি (!toVarrefp) ঘূৰাই মিছা;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        যদি (fromVarrefp->varScopep() != toVarrefp->varScopep()) ঘূৰাই মিছা;

        ঘূৰাই fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    শূন্য addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        নূতন V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    শূন্য iterateLogicVertex(ধ্রুৱ OrderLogicVertex* lvtxp) {
        ধ্রুৱ OrderDomain* ধ্রুৱ domainp = lvtxp->domainp();
        OrderMoveVertex* ধ্রুৱ lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        বাবে (ধ্রুৱ V3GraphEdge& edge : lvtxp->outEdges()) {
            যদি (edge.weight() == 0) অনুৱর্ত;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            ধ্রুৱ OrderVarVertex* ধ্রুৱ vvtxp = static_cast<ধ্রুৱ OrderVarVertex*>(edge.শীৰ্ষ());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            ধ্রুৱ auto pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            যদি (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            যদি (!vMoveVtxp) অনুৱর্ত;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(ধ্রুৱ OrderVarVertex* vvtxp, ধ্রুৱ OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        বাবে (ধ্রুৱ V3GraphEdge& edge : vvtxp->outEdges()) {
            যদি (edge.weight() == 0) অনুৱর্ত;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            ধ্রুৱ OrderLogicVertex* ধ্রুৱ lVtxp = edge.শীৰ্ষ()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            যদি (domainsExclusive(domainp, lVtxp->domainp())) অনুৱর্ত;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            যদি (!vMoveVtxp) vMoveVtxp = নূতন OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* ধ্রুৱ lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        ঘূৰাই vMoveVtxp;
    }

সার্ৱজনিক:
    স্থির std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        ঘূৰাই std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    ঘূৰাই OrderMoveGraphBuilder::apply(orderGraph);
}
