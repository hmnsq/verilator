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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3Order.h"
#ضمن "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* ثابت m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ثابت override { ܦܢܐ logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ثابت override VL_MT_STABLE {
        std::string nm;
        ܐܢ (!logicp()) {
            nm = "var";
        } ܐܚܪܢܐ {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        ܐܢ (userp()) nm += "\nu=" + cvtToHex(userp());
        ܦܢܐ nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { ܦܢܐ m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    ثابت OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = افتراضي;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() ثابت VL_MT_STABLE { ܦܢܐ m_logicp; }
    OrderMoveDomScope& domScope() ثابت { ܦܢܐ m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : عام V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

عام:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() ثابت { ܦܢܐ m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    ساكن std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    ثابت OrderDomain* ثابت m_domainp;  // Domain the vertices belong to
    ثابت AstScope* ثابت m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class DomScopeMapKey final {
        ثابت OrderDomain* ثابت m_domainp;  // Domain the vertices belong to
        ثابت AstScope* ثابت m_scopep;  // Scope the vertices belong to

    عام:
        DomScopeMapKey(ثابت OrderDomain* domainp, ثابت AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        بنية Hash final {
            size_t operator()(ثابت DomScopeMapKey& key) ثابت {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{key.m_domainp};
                hash += key.m_scopep;
                ܦܢܐ hash.value();
            }
        };

        بنية Equal final {
            bool operator()(ثابت DomScopeMapKey& a, ثابت DomScopeMapKey& b) ثابت {
                ܦܢܐ a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<DomScopeMapKey, OrderMoveDomScope, DomScopeMapKey::Hash,
                                           DomScopeMapKey::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ساكن DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { ܦܢܐ m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    ساكن OrderMoveDomScope& getOrCreate(ثابت OrderDomain* domainp, ثابت AstScope* scopep) {
        ܦܢܐ s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    ساكن فراغ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(ثابت OrderDomain* domainp, ثابت AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = افتراضي;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { ܦܢܐ m_readyVertices; }
    ثابت OrderDomain* domainp() ثابت { ܦܢܐ m_domainp; }
    ثابت AstScope* scopep() ثابت { ܦܢܐ m_scopep; }

    bool isOnList() ثابت { ܦܢܐ m_isOnList; }
    فراغ isOnList(bool value) { m_isOnList = value; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    فراغ ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        ܐܢ (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            ܐܢ (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } ܐܚܪܢܐ {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            لكل (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* ثابت dVtxp = edge.top()->as<OrderMoveVertex>();
                // Update number of dependencies
                ثابت uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                ܐܢ (!nDeps) ready(dVtxp);
            }
        }
    }

عام:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        لكل (V3GraphVertex& vtx : moveGraph.vertices()) {
            ثابت uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = افتراضي;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    فراغ addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        ܐܢ (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        ܐܢ (!m_nextDomScopep) ܦܢܐ nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        ܐܢ (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        لكل (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* ثابت dVtxp = edge.top()->as<OrderMoveVertex>();
            // Update number of dependencies
            ثابت uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            ܐܢ (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        ܐܢ (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            لكل (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                ܐܢ (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    اكسر;
                }
            }
        }

        // Finally yield the selected vertex
        ܦܢܐ mVtxp;
    }
};

#endif  // Guard
