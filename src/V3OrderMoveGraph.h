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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Order.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* ଅଚଳ m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ଅଚଳ override VL_MT_STABLE {
        std::string nm;
        ଯଦି (!logicp()) {
            nm = "var";
        } ଅନ୍ୟଥା {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        ଯଦି (userp()) nm += "\nu=" + cvtToHex(userp());
        ଫେରନ୍ତୁ nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { ଫେରନ୍ତୁ m_links; }

ସର୍ବସାଧାରଣ:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    ଅଚଳ OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_logicp; }
    OrderMoveDomScope& domScope() ଅଚଳ { ଫେରନ୍ତୁ m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : ସର୍ବସାଧାରଣ V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

ସର୍ବସାଧାରଣ:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() ଅଚଳ { ଫେରନ୍ତୁ m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    ସ୍ଥିର std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    ଅଚଳ OrderDomain* ଅଚଳ m_domainp;  // Domain the vertices belong to
    ଅଚଳ AstScope* ଅଚଳ m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_ଚାବି final {
        ଅଚଳ OrderDomain* ଅଚଳ m_domainp;  // Domain the vertices belong to
        ଅଚଳ AstScope* ଅଚଳ m_scopep;  // Scope the vertices belong to

    ସର୍ବସାଧାରଣ:
        Dom_Scope_Map_ଚାବି(ଅଚଳ OrderDomain* domainp, ଅଚଳ AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        ସଂରଚନା Hash final {
            size_t operator()(ଅଚଳ Dom_Scope_Map_ଚାବି& ଚାବି) ଅଚଳ {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{ଚାବି.m_domainp};
                hash += ଚାବି.m_scopep;
                ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
            }
        };

        ସଂରଚନା Equal final {
            bool operator()(ଅଚଳ Dom_Scope_Map_ଚାବି& a, ଅଚଳ Dom_Scope_Map_ଚାବି& b) ଅଚଳ {
                ଫେରନ୍ତୁ a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_ଚାବି, OrderMoveDomScope, Dom_Scope_Map_ଚାବି::Hash,
                                           Dom_Scope_Map_ଚାବି::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ସ୍ଥିର DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { ଫେରନ୍ତୁ m_links; }

ସର୍ବସାଧାରଣ:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    ସ୍ଥିର OrderMoveDomScope& getOrCreate(ଅଚଳ OrderDomain* domainp, ଅଚଳ AstScope* scopep) {
        ଫେରନ୍ତୁ s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    ସ୍ଥିର ଶୂନ୍ୟ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(ଅଚଳ OrderDomain* domainp, ଅଚଳ AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { ଫେରନ୍ତୁ m_readyVertices; }
    ଅଚଳ OrderDomain* domainp() ଅଚଳ { ଫେରନ୍ତୁ m_domainp; }
    ଅଚଳ AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }

    bool isOnList() ଅଚଳ { ଫେରନ୍ତୁ m_isOnList; }
    ଶୂନ୍ୟ isOnList(bool ମୂଲ୍ୟ) { m_isOnList = ମୂଲ୍ୟ; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    ଶୂନ୍ୟ ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        ଯଦି (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            ଯଦି (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } ଅନ୍ୟଥା {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            ପାଇଁ (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* ଅଚଳ dVtxp = edge.ଶୀର୍ଷ()->as<OrderMoveVertex>();
                // Update number of dependencies
                ଅଚଳ uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                ଯଦି (!nDeps) ready(dVtxp);
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        ପାଇଁ (V3GraphVertex& vtx : moveGraph.vertices()) {
            ଅଚଳ uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    ଶୂନ୍ୟ addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        ଯଦି (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        ଯଦି (!m_nextDomScopep) ଫେରନ୍ତୁ nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        ଯଦି (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        ପାଇଁ (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* ଅଚଳ dVtxp = edge.ଶୀର୍ଷ()->as<OrderMoveVertex>();
            // Update number of dependencies
            ଅଚଳ uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            ଯଦି (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        ଯଦି (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            ପାଇଁ (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                ଯଦି (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    ଭାଙ୍ଗ;
                }
            }
        }

        // Finally yield the selected vertex
        ଫେରନ୍ତୁ mVtxp;
    }
};

#endif  // Guard
