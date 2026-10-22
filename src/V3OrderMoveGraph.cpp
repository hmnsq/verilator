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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3OrderMoveGraph.h"

#അന്തര്ഭാവ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// OrderMoveDomScope implementation

OrderMoveDomScope::DomScopeMap OrderMoveDomScope::s_dsMap;

//======================================================================
// OrderMoveVertex implementation

OrderMoveVertex::OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                                 സ്ഥിരം OrderDomain* domainp) VL_MT_DISABLED
    : V3GraphVertex{&graph},
      m_logicp{lVtxp},
      m_domScope{OrderMoveDomScope::getOrCreate(domainp, lVtxp ? lVtxp->scopep() : nullptr)} {
    UASSERT_OBJ(!lVtxp || lVtxp->domainp() == domainp, lVtxp, "Wrong domain for Move vertex");
}

//======================================================================
// OrderMoveGraphBuilder - for OrderMoveGraph::build

വർഗ്ഗം OrderMoveGraphBuilder final {
    // TYPES
    using DomainMap = std::map<സ്ഥിരം OrderDomain*, OrderMoveVertex*>;

    // MEMBERS
    OrderGraph& m_orderGraph;  // Input OrderGraph
    // Output OrderMoveGraph
    std::unique_ptr<OrderMoveGraph> m_moveGraphp{പുതിയ OrderMoveGraph{m_orderGraph}};
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
        വേണ്ടി (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            എങ്കിൽ (OrderLogicVertex* സ്ഥിരം lvtxp = vtx.cast<OrderLogicVertex>()) {
                lvtxp->userp(പുതിയ OrderMoveVertex{*m_moveGraphp, lvtxp, lvtxp->domainp()});
            } അല്ലെങ്കിൽ {
                // This is an OrderVarVertex
                m_domainMaps.emplace_back();
                vtx.userp(&m_domainMaps.back());
            }
        }
        // Build edges between logic vertices
        വേണ്ടി (V3GraphVertex& vtx : m_orderGraph.vertices()) {
            എങ്കിൽ (OrderLogicVertex* സ്ഥിരം lvtxp = vtx.cast<OrderLogicVertex>()) {
                iterateLogicVertex(lvtxp);
            }
        }
        m_moveGraphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        m_moveGraphp->userClearVertices();
    }
    മായാ ~OrderMoveGraphBuilder() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(OrderMoveGraphBuilder);
    VL_UNMOVABLE(OrderMoveGraphBuilder);

    // METHODS

    തർക്കം domainsExclusive(സ്ഥിരം OrderDomain* fromp, സ്ഥിരം OrderDomain* മുകൾ) {
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

        സ്ഥിരം std::set<uint32_t>& fromTriggers = fromp->triggers();
        സ്ഥിരം std::set<uint32_t>& toTriggers = മുകൾ->triggers();
        എങ്കിൽ (fromTriggers.size() != 1) തിരികെ അസത്യം;
        എങ്കിൽ (toTriggers.size() != 1) തിരികെ അസത്യം;
        സ്ഥിരം AstSenItem* സ്ഥിരം fromSenItemp = m_orderGraph.trigSenItemp(*fromTriggers.begin());
        എങ്കിൽ (!fromSenItemp) തിരികെ അസത്യം;
        സ്ഥിരം AstSenItem* സ്ഥിരം toSenItemp = m_orderGraph.trigSenItemp(*toTriggers.begin());
        എങ്കിൽ (!toSenItemp) തിരികെ അസത്യം;

        സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം fromVarrefp = fromSenItemp->varrefp();
        എങ്കിൽ (!fromVarrefp) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം toVarrefp = toSenItemp->varrefp();
        എങ്കിൽ (!toVarrefp) തിരികെ അസത്യം;

        // We know nothing about the relationship between different clocks here,
        // so only proceed if strictly the same clock.
        എങ്കിൽ (fromVarrefp->varScopep() != toVarrefp->varScopep()) തിരികെ അസത്യം;

        തിരികെ fromSenItemp->edgeType().exclusiveEdge(toSenItemp->edgeType());
    }

    ശൂന്യം addEdge(OrderMoveVertex* srcp, OrderMoveVertex* dstp) {
        പുതിയ V3GraphEdge{m_moveGraphp.get(), srcp, dstp, 1};
    }

    ശൂന്യം iterateLogicVertex(സ്ഥിരം OrderLogicVertex* lvtxp) {
        സ്ഥിരം OrderDomain* സ്ഥിരം domainp = lvtxp->domainp();
        OrderMoveVertex* സ്ഥിരം lMoveVtxp = static_cast<OrderMoveVertex*>(lvtxp->userp());
        // Search forward from lvtxp, making new edges from lMoveVtxp forward
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : lvtxp->outEdges()) {
            എങ്കിൽ (edge.weight() == 0) തുടരുക;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderVarVertex
            സ്ഥിരം OrderVarVertex* സ്ഥിരം vvtxp = static_cast<സ്ഥിരം OrderVarVertex*>(edge.മുകൾ());

            // Look up OrderMoveVertex for this domain on this variable
            DomainMap& mapp = *static_cast<DomainMap*>(vvtxp->userp());
            സ്ഥിരം auto pair = mapp.emplace(domainp, nullptr);
            // Reference to the mapped OrderMoveVertex
            OrderMoveVertex*& vMoveVtxp = pair.first->second;

            // On first encounter, visit downstream logic dependent on this (var, domain)
            എങ്കിൽ (pair.second) vMoveVtxp = iterateVarVertex(vvtxp, domainp);

            // If no downstream dependents from this variable, then there is no need to add this
            // variable as a dependent.
            എങ്കിൽ (!vMoveVtxp) തുടരുക;

            // Add this (variable, domain) as dependent of the logic that writes it.
            addEdge(lMoveVtxp, vMoveVtxp);
        }
    }

    // Return the OrderMoveVertex for this (var, domain) pair, iff it has downstream dependencies,
    // otherwise return nullptr.
    OrderMoveVertex* iterateVarVertex(സ്ഥിരം OrderVarVertex* vvtxp, സ്ഥിരം OrderDomain* domainp) {
        OrderMoveVertex* vMoveVtxp = nullptr;
        // Search forward from vvtxp, making new edges from vMoveVtxp forward
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vvtxp->outEdges()) {
            എങ്കിൽ (edge.weight() == 0) തുടരുക;  // Was cut

            // OrderGraph is a bipartite graph, so we know it's an OrderLogicVertex
            സ്ഥിരം OrderLogicVertex* സ്ഥിരം lVtxp = edge.മുകൾ()->as<OrderLogicVertex>();

            // Do not construct dependencies across exclusive domains.
            എങ്കിൽ (domainsExclusive(domainp, lVtxp->domainp())) തുടരുക;

            // there is a path from this vvtx to a logic vertex. Add the new edge.
            എങ്കിൽ (!vMoveVtxp) vMoveVtxp = പുതിയ OrderMoveVertex{*m_moveGraphp, nullptr, domainp};
            OrderMoveVertex* സ്ഥിരം lMoveVxp = static_cast<OrderMoveVertex*>(lVtxp->userp());
            addEdge(vMoveVtxp, lMoveVxp);
        }
        തിരികെ vMoveVtxp;
    }

പൊതു:
    സ്ഥിതം std::unique_ptr<OrderMoveGraph> apply(OrderGraph& orderGraph) {
        തിരികെ std::move(OrderMoveGraphBuilder{orderGraph}.m_moveGraphp);
    }
};

//======================================================================
// OrderMoveGraph implementation

std::unique_ptr<OrderMoveGraph> OrderMoveGraph::build(OrderGraph& orderGraph) {
    തിരികെ OrderMoveGraphBuilder::apply(orderGraph);
}
