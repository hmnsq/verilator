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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3OrderMoveGraph.h"

#समावेश "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 स्थिर OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

श्रेणी OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<स्थिर OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{नया OrderMoveGraph{m_orderGraph}};
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
        क्रम (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            अगर (OrderLogicVertex* स्थिर lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(नया OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } अन्यथा {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        क्रम (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            अगर (OrderLogicVertex* स्थिर lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->remove_Redundant_Edges_योग(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    भव ~OrderMoveGraphBuilder() = शेष;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    bool domainsExclusive(स्थिर OrderDomain* fromp, स्थिर OrderDomain* शीर्ष) {
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

        स्थिर std::set<uint32_t>& fromTriggers = fromp->triggers();
        स्थिर std::set<uint32_t>& toTriggers = शीर्ष->triggers();
        अगर (fromTriggers.आकार() != 1) वापस false;
        अगर (toTriggers.आकार() != 1) वापस false;
        स्थिर AstSenItem* स्थिर fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        अगर (!fromSenItemp) वापस false;
        स्थिर AstSenItem* स्थिर toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        अगर (!toSenItemp) वापस false;

        स्थिर Ast_गाँठ_Var_Ref* स्थिर fromVarrefp = fromSenItemp->varrefp();
        अगर (!fromVarrefp) वापस false;
        स्थिर Ast_गाँठ_Var_Ref* स्थिर toVarrefp = toSenItemp->varrefp();
        अगर (!toVarrefp) वापस false;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        अगर (fromVarrefp->varScopep() != toVarrefp->varScopep()) वापस false;

        वापस fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    व्योम addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        नया V3GraphEdge{m_moveGraphp.लो(), srcp, dstp, 1};
    }

    व्योम iterateLogicVertex(स्थिर OrderLogicVertex* lvtxp) {
        स्थिर OrderDomain* स्थिर domainp = lvtxp->domainp();
        OrderMoveVertex* स्थिर lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        क्रम (स्थिर V3GraphEdge& edge : lvtxp->outEdges()) {
            अगर (edge.weight() == 0) जारी;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            स्थिर OrderVarVertex* स्थिर vvtxp = static_cast<स्थिर OrderVarVertex*>(edge.शीर्ष());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            स्थिर स्वतः pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            अगर (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            अगर (!vMoveVtxp) जारी;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(स्थिर OrderVarVertex* vvtxp, स्थिर OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        क्रम (स्थिर V3GraphEdge& edge : vvtxp->outEdges()) {
            अगर (edge.weight() == 0) जारी;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            स्थिर OrderLogicVertex* स्थिर lVtxp = edge.शीर्ष()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            अगर (domainsExclusive(domainp, lVtxp->domainp())) जारी;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            अगर (!vMoveVtxp) vMoveVtxp = नया OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* स्थिर lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        वापस vMoveVtxp;
    }

खुला:
    अटल std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        वापस std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    वापस OrderMoveGraphBuilder::apply(orderGraph);
}
