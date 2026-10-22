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
//  Move graph builder for ordering
//
//*************************************************************************

#ifndef VERILATOR_V3ORDERMOVEGRAPH_H_
#define VERILATOR_V3ORDERMOVEGRAPH_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* ಧ್ರುವ m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ಧ್ರುವ override VL_MT_STABLE {
        std::string nm;
        ಯದಿ (!logicp()) {
            nm = "var";
        } ಅನ್ಯಥಾ {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        ಯದಿ (userp()) nm += "\nu=" + cvtToHex(userp());
        ಪ್ರತ್ಯಾಗಮ nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { ಪ್ರತ್ಯಾಗಮ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    ಧ್ರುವ OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_logicp; }
    OrderMoveDomScope& domScope() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

ಸಾರ್ವಜನಿಕ:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    ಸ್ಥಿರ std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class OrderMoveDomScope final {
    // STATE
    OrderMoveVertex::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<OrderMoveDomScope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    ಧ್ರುವ OrderDomain* ಧ್ರುವ m_domainp;  // Domain the vertices belong to
    ಧ್ರುವ AstScope* ಧ್ರುವ m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_ಕೀಲಿ final {
        ಧ್ರುವ OrderDomain* ಧ್ರುವ m_domainp;  // Domain the vertices belong to
        ಧ್ರುವ AstScope* ಧ್ರುವ m_scopep;  // Scope the vertices belong to

    ಸಾರ್ವಜನಿಕ:
        Dom_Scope_Map_ಕೀಲಿ(ಧ್ರುವ OrderDomain* domainp, ಧ್ರುವ AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        ಸಂರಚನಾ Hash final {
            size_t operator()(ಧ್ರುವ Dom_Scope_Map_ಕೀಲಿ& ಕೀಲಿ) ಧ್ರುವ {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{ಕೀಲಿ.m_domainp};
                hash += ಕೀಲಿ.m_scopep;
                ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
            }
        };

        ಸಂರಚನಾ Equal final {
            bool operator()(ಧ್ರುವ Dom_Scope_Map_ಕೀಲಿ& a, ಧ್ರುವ Dom_Scope_Map_ಕೀಲಿ& b) ಧ್ರುವ {
                ಪ್ರತ್ಯಾಗಮ a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_ಕೀಲಿ, OrderMoveDomScope, Dom_Scope_Map_ಕೀಲಿ::Hash,
                                           Dom_Scope_Map_ಕೀಲಿ::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ಸ್ಥಿರ DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { ಪ್ರತ್ಯಾಗಮ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    ಸ್ಥಿರ OrderMoveDomScope& getOrCreate(ಧ್ರುವ OrderDomain* domainp, ಧ್ರುವ AstScope* scopep) {
        ಪ್ರತ್ಯಾಗಮ s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(ಧ್ರುವ OrderDomain* domainp, ಧ್ರುವ AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { ಪ್ರತ್ಯಾಗಮ m_readyVertices; }
    ಧ್ರುವ OrderDomain* domainp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_domainp; }
    ಧ್ರುವ AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }

    bool isOnList() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isOnList; }
    ಶೂನ್ಯ isOnList(bool ಮೌಲ್ಯ) { m_isOnList = ಮೌಲ್ಯ; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    ಶೂನ್ಯ ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        ಯದಿ (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            ಯದಿ (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } ಅನ್ಯಥಾ {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            ಪ್ರತಿ (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* ಧ್ರುವ dVtxp = edge.ತುದಿ()->as<OrderMoveVertex>();
                // Update number of dependencies
                ಧ್ರುವ uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                ಯದಿ (!nDeps) ready(dVtxp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        ಪ್ರತಿ (V3GraphVertex& vtx : moveGraph.vertices()) {
            ಧ್ರುವ uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    ಶೂನ್ಯ addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        ಯದಿ (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        ಯದಿ (!m_nextDomScopep) ಪ್ರತ್ಯಾಗಮ nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        ಯದಿ (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        ಪ್ರತಿ (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* ಧ್ರುವ dVtxp = edge.ತುದಿ()->as<OrderMoveVertex>();
            // Update number of dependencies
            ಧ್ರುವ uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            ಯದಿ (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        ಯದಿ (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            ಪ್ರತಿ (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                ಯದಿ (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    ಭಙ್ಗ;
                }
            }
        }

        // Finally yield the selected vertex
        ಪ್ರತ್ಯಾಗಮ mVtxp;
    }
};

#endif  // Guard
