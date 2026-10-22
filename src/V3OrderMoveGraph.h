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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Order.h"
#அந்தர்பாவ "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* மாறிலி m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() மாறிலி override { திரும்பு logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() மாறிலி override VL_MT_STABLE {
        std::string nm;
        எனில் (!logicp()) {
            nm = "var";
        } இல்லையெனில் {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        எனில் (userp()) nm += "\nu=" + cvtToHex(userp());
        திரும்பு nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { திரும்பு m_links; }

பொது:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    மாறிலி OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = இயல்பு;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() மாறிலி VL_MT_STABLE { திரும்பு m_logicp; }
    OrderMoveDomScope& domScope() மாறிலி { திரும்பு m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : பொது V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

பொது:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() மாறிலி { திரும்பு m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    நிலையான std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    மாறிலி OrderDomain* மாறிலி m_domainp;  // Domain the vertices belong to
    மாறிலி AstScope* மாறிலி m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_சாவி final {
        மாறிலி OrderDomain* மாறிலி m_domainp;  // Domain the vertices belong to
        மாறிலி AstScope* மாறிலி m_scopep;  // Scope the vertices belong to

    பொது:
        Dom_Scope_Map_சாவி(மாறிலி OrderDomain* domainp, மாறிலி AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        கட்டமைப்பு Hash final {
            size_t operator()(மாறிலி Dom_Scope_Map_சாவி& சாவி) மாறிலி {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{சாவி.m_domainp};
                hash += சாவி.m_scopep;
                திரும்பு hash.மதிப்பு();
            }
        };

        கட்டமைப்பு Equal final {
            bool operator()(மாறிலி Dom_Scope_Map_சாவி& a, மாறிலி Dom_Scope_Map_சாவி& b) மாறிலி {
                திரும்பு a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_சாவி, OrderMoveDomScope, Dom_Scope_Map_சாவி::Hash,
                                           Dom_Scope_Map_சாவி::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    நிலையான DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { திரும்பு m_links; }

பொது:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    நிலையான OrderMoveDomScope& getOrCreate(மாறிலி OrderDomain* domainp, மாறிலி AstScope* scopep) {
        திரும்பு s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    நிலையான வெறுமை clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(மாறிலி OrderDomain* domainp, மாறிலி AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = இயல்பு;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { திரும்பு m_readyVertices; }
    மாறிலி OrderDomain* domainp() மாறிலி { திரும்பு m_domainp; }
    மாறிலி AstScope* scopep() மாறிலி { திரும்பு m_scopep; }

    bool isOnList() மாறிலி { திரும்பு m_isOnList; }
    வெறுமை isOnList(bool மதிப்பு) { m_isOnList = மதிப்பு; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    வெறுமை ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        எனில் (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            எனில் (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } இல்லையெனில் {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            க்கு (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* மாறிலி dVtxp = edge.உச்சி()->as<OrderMoveVertex>();
                // Update number of dependencies
                மாறிலி uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                எனில் (!nDeps) ready(dVtxp);
            }
        }
    }

பொது:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        க்கு (V3GraphVertex& vtx : moveGraph.vertices()) {
            மாறிலி uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = இயல்பு;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    வெறுமை addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        எனில் (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        எனில் (!m_nextDomScopep) திரும்பு nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        எனில் (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        க்கு (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* மாறிலி dVtxp = edge.உச்சி()->as<OrderMoveVertex>();
            // Update number of dependencies
            மாறிலி uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            எனில் (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        எனில் (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            க்கு (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                எனில் (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    நிறுத்து;
                }
            }
        }

        // Finally yield the selected vertex
        திரும்பு mVtxp;
    }
};

#endif  // Guard
