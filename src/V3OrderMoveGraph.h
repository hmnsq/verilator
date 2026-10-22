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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Order.h"
#અન્તર્ભાવ "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* અચલ m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() અચલ override { પાછા logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() અચલ override VL_MT_STABLE {
        std::string nm;
        જો (!logicp()) {
            nm = "var";
        } નહીંતર {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        જો (userp()) nm += "\nu=" + cvtToHex(userp());
        પાછા nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { પાછા m_links; }

જાહેર:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    અચલ OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = મૂળભૂત;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() અચલ VL_MT_STABLE { પાછા m_logicp; }
    OrderMoveDomScope& domScope() અચલ { પાછા m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : જાહેર V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

જાહેર:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() અચલ { પાછા m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    સ્થિર std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    અચલ OrderDomain* અચલ m_domainp;  // Domain the vertices belong to
    અચલ AstScope* અચલ m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_ચાવી final {
        અચલ OrderDomain* અચલ m_domainp;  // Domain the vertices belong to
        અચલ AstScope* અચલ m_scopep;  // Scope the vertices belong to

    જાહેર:
        Dom_Scope_Map_ચાવી(અચલ OrderDomain* domainp, અચલ AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        સંરચના Hash final {
            size_t operator()(અચલ Dom_Scope_Map_ચાવી& ચાવી) અચલ {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{ચાવી.m_domainp};
                hash += ચાવી.m_scopep;
                પાછા hash.કિંમત();
            }
        };

        સંરચના Equal final {
            bool operator()(અચલ Dom_Scope_Map_ચાવી& a, અચલ Dom_Scope_Map_ચાવી& b) અચલ {
                પાછા a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_ચાવી, OrderMoveDomScope, Dom_Scope_Map_ચાવી::Hash,
                                           Dom_Scope_Map_ચાવી::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    સ્થિર DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { પાછા m_links; }

જાહેર:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    સ્થિર OrderMoveDomScope& getOrCreate(અચલ OrderDomain* domainp, અચલ AstScope* scopep) {
        પાછા s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    સ્થિર શૂન્ય clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(અચલ OrderDomain* domainp, અચલ AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = મૂળભૂત;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { પાછા m_readyVertices; }
    અચલ OrderDomain* domainp() અચલ { પાછા m_domainp; }
    અચલ AstScope* scopep() અચલ { પાછા m_scopep; }

    bool isOnList() અચલ { પાછા m_isOnList; }
    શૂન્ય isOnList(bool કિંમત) { m_isOnList = કિંમત; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    શૂન્ય ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        જો (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            જો (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } નહીંતર {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            માટે (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* અચલ dVtxp = edge.ટોચ()->as<OrderMoveVertex>();
                // Update number of dependencies
                અચલ uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                જો (!nDeps) ready(dVtxp);
            }
        }
    }

જાહેર:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        માટે (V3GraphVertex& vtx : moveGraph.vertices()) {
            અચલ uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = મૂળભૂત;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    શૂન્ય addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        જો (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        જો (!m_nextDomScopep) પાછા nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        જો (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        માટે (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* અચલ dVtxp = edge.ટોચ()->as<OrderMoveVertex>();
            // Update number of dependencies
            અચલ uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            જો (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        જો (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            માટે (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                જો (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    તોડો;
                }
            }
        }

        // Finally yield the selected vertex
        પાછા mVtxp;
    }
};

#endif  // Guard
