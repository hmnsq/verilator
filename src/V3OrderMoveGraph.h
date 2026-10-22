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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Graph.h"
#include "V3Order.h"
#include "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : public V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* const m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() const override { ལོག logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() const override VL_MT_STABLE {
        std::string nm;
        གལ་སྲིད (!logicp()) {
            nm = "var";
        } དེ་མིན {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        གལ་སྲིད (userp()) nm += "\nu=" + cvtToHex(userp());
        ལོག nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { ལོག m_links; }

public:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    const OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = default;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() const VL_MT_STABLE { ལོག m_logicp; }
    OrderMoveDomScope& domScope() const { ལོག m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : public V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

public:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() const { ལོག m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    static std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    const OrderDomain* const m_domainp;  // Domain the vertices belong to
    const AstScope* const m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class DomScopeMapKey final {
        const OrderDomain* const m_domainp;  // Domain the vertices belong to
        const AstScope* const m_scopep;  // Scope the vertices belong to

    public:
        DomScopeMapKey(const OrderDomain* domainp, const AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        sa_nracanaa Hash final {
            size_t operator()(const DomScopeMapKey& key) const {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{key.m_domainp};
                hash += key.m_scopep;
                ལོག hash.value();
            }
        };

        sa_nracanaa Equal final {
            bool operator()(const DomScopeMapKey& a, const DomScopeMapKey& b) const {
                ལོག a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<DomScopeMapKey, OrderMoveDomScope, DomScopeMapKey::Hash,
                                           DomScopeMapKey::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    static DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { ལོག m_links; }

public:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    static OrderMoveDomScope& getOrCreate(const OrderDomain* domainp, const AstScope* scopep) {
        ལོག s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    static void clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(const OrderDomain* domainp, const AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = default;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { ལོག m_readyVertices; }
    const OrderDomain* domainp() const { ལོག m_domainp; }
    const AstScope* scopep() const { ལོག m_scopep; }

    bool isOnList() const { ལོག m_isOnList; }
    void isOnList(bool value) { m_isOnList = value; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    void ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        གལ་སྲིད (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            གལ་སྲིད (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } དེ་མིན {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            དོན་ལུ (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* const dVtxp = edge.top()->as<OrderMoveVertex>();
                // Update number of dependencies
                const uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                གལ་སྲིད (!nDeps) ready(dVtxp);
            }
        }
    }

public:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        དོན་ལུ (V3GraphVertex& vtx : moveGraph.vertices()) {
            const uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = default;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    void addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        གལ་སྲིད (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        གལ་སྲིད (!m_nextDomScopep) ལོག nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        གལ་སྲིད (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        དོན་ལུ (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* const dVtxp = edge.top()->as<OrderMoveVertex>();
            // Update number of dependencies
            const uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            གལ་སྲིད (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        གལ་སྲིད (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            དོན་ལུ (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                གལ་སྲིད (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    bha_nga;
                }
            }
        }

        // Finally yield the selected vertex
        ལོག mVtxp;
    }
};

#endif  // Guard
