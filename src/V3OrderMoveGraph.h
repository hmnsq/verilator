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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Order.h"
#അന്തര്ഭാവ "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* സ്ഥിരം m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() സ്ഥിരം override { തിരികെ logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() സ്ഥിരം override VL_MT_STABLE {
        std::string nm;
        എങ്കിൽ (!logicp()) {
            nm = "var";
        } അല്ലെങ്കിൽ {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        എങ്കിൽ (userp()) nm += "\nu=" + cvtToHex(userp());
        തിരികെ nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { തിരികെ m_links; }

പൊതു:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    സ്ഥിരം OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_logicp; }
    OrderMoveDomScope& domScope() സ്ഥിരം { തിരികെ m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : പൊതു V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

പൊതു:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() സ്ഥിരം { തിരികെ m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    സ്ഥിതം std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    സ്ഥിരം OrderDomain* സ്ഥിരം m_domainp;  // Domain the vertices belong to
    സ്ഥിരം AstScope* സ്ഥിരം m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_താക്കോൽ final {
        സ്ഥിരം OrderDomain* സ്ഥിരം m_domainp;  // Domain the vertices belong to
        സ്ഥിരം AstScope* സ്ഥിരം m_scopep;  // Scope the vertices belong to

    പൊതു:
        Dom_Scope_Map_താക്കോൽ(സ്ഥിരം OrderDomain* domainp, സ്ഥിരം AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        ഘടന Hash final {
            size_t operator()(സ്ഥിരം Dom_Scope_Map_താക്കോൽ& താക്കോൽ) സ്ഥിരം {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{താക്കോൽ.m_domainp};
                hash += താക്കോൽ.m_scopep;
                തിരികെ hash.മൂല്യം();
            }
        };

        ഘടന Equal final {
            bool operator()(സ്ഥിരം Dom_Scope_Map_താക്കോൽ& a, സ്ഥിരം Dom_Scope_Map_താക്കോൽ& b) സ്ഥിരം {
                തിരികെ a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_താക്കോൽ, OrderMoveDomScope, Dom_Scope_Map_താക്കോൽ::Hash,
                                           Dom_Scope_Map_താക്കോൽ::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    സ്ഥിതം DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { തിരികെ m_links; }

പൊതു:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    സ്ഥിതം OrderMoveDomScope& getOrCreate(സ്ഥിരം OrderDomain* domainp, സ്ഥിരം AstScope* scopep) {
        തിരികെ s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    സ്ഥിതം ശൂന്യം clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(സ്ഥിരം OrderDomain* domainp, സ്ഥിരം AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { തിരികെ m_readyVertices; }
    സ്ഥിരം OrderDomain* domainp() സ്ഥിരം { തിരികെ m_domainp; }
    സ്ഥിരം AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }

    bool isOnList() സ്ഥിരം { തിരികെ m_isOnList; }
    ശൂന്യം isOnList(bool മൂല്യം) { m_isOnList = മൂല്യം; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    ശൂന്യം ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        എങ്കിൽ (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            എങ്കിൽ (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } അല്ലെങ്കിൽ {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            വേണ്ടി (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* സ്ഥിരം dVtxp = edge.മുകൾ()->as<OrderMoveVertex>();
                // Update number of dependencies
                സ്ഥിരം uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                എങ്കിൽ (!nDeps) ready(dVtxp);
            }
        }
    }

പൊതു:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        വേണ്ടി (V3GraphVertex& vtx : moveGraph.vertices()) {
            സ്ഥിരം uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    ശൂന്യം addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        എങ്കിൽ (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        എങ്കിൽ (!m_nextDomScopep) തിരികെ nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        എങ്കിൽ (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        വേണ്ടി (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* സ്ഥിരം dVtxp = edge.മുകൾ()->as<OrderMoveVertex>();
            // Update number of dependencies
            സ്ഥിരം uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            എങ്കിൽ (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        എങ്കിൽ (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            വേണ്ടി (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                എങ്കിൽ (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    നിർത്തുക;
                }
            }
        }

        // Finally yield the selected vertex
        തിരികെ mVtxp;
    }
};

#endif  // Guard
