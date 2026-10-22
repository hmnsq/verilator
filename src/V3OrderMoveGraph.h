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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Order.h"
#অন্তর্ভাব "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* ধ্রুব m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ধ্রুব override { প্রত্যাগম logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ধ্রুব override VL_MT_STABLE {
        std::string nm;
        যদি (!logicp()) {
            nm = "var";
        } অন্যথা {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        যদি (userp()) nm += "\nu=" + cvtToHex(userp());
        প্রত্যাগম nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { প্রত্যাগম m_links; }

সার্বজনিক:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    ধ্রুব OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = স্বভাব;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_logicp; }
    OrderMoveDomScope& domScope() ধ্রুব { প্রত্যাগম m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : সার্বজনিক V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

সার্বজনিক:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() ধ্রুব { প্রত্যাগম m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    স্থির std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    ধ্রুব OrderDomain* ধ্রুব m_domainp;  // Domain the vertices belong to
    ধ্রুব AstScope* ধ্রুব m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_চাবি final {
        ধ্রুব OrderDomain* ধ্রুব m_domainp;  // Domain the vertices belong to
        ধ্রুব AstScope* ধ্রুব m_scopep;  // Scope the vertices belong to

    সার্বজনিক:
        Dom_Scope_Map_চাবি(ধ্রুব OrderDomain* domainp, ধ্রুব AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        সংরচনা Hash final {
            size_t operator()(ধ্রুব Dom_Scope_Map_চাবি& চাবি) ধ্রুব {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{চাবি.m_domainp};
                hash += চাবি.m_scopep;
                প্রত্যাগম hash.মান();
            }
        };

        সংরচনা Equal final {
            bool operator()(ধ্রুব Dom_Scope_Map_চাবি& a, ধ্রুব Dom_Scope_Map_চাবি& b) ধ্রুব {
                প্রত্যাগম a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_চাবি, OrderMoveDomScope, Dom_Scope_Map_চাবি::Hash,
                                           Dom_Scope_Map_চাবি::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    স্থির DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { প্রত্যাগম m_links; }

সার্বজনিক:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    স্থির OrderMoveDomScope& getOrCreate(ধ্রুব OrderDomain* domainp, ধ্রুব AstScope* scopep) {
        প্রত্যাগম s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    স্থির শূন্য clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(ধ্রুব OrderDomain* domainp, ধ্রুব AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = স্বভাব;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { প্রত্যাগম m_readyVertices; }
    ধ্রুব OrderDomain* domainp() ধ্রুব { প্রত্যাগম m_domainp; }
    ধ্রুব AstScope* scopep() ধ্রুব { প্রত্যাগম m_scopep; }

    bool isOnList() ধ্রুব { প্রত্যাগম m_isOnList; }
    শূন্য isOnList(bool মান) { m_isOnList = মান; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    শূন্য ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        যদি (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            যদি (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } অন্যথা {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            প্রতি (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* ধ্রুব dVtxp = edge.শীর্ষ()->as<OrderMoveVertex>();
                // Update number of dependencies
                ধ্রুব uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                যদি (!nDeps) ready(dVtxp);
            }
        }
    }

সার্বজনিক:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        প্রতি (V3GraphVertex& vtx : moveGraph.vertices()) {
            ধ্রুব uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = স্বভাব;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    শূন্য addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        যদি (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        যদি (!m_nextDomScopep) প্রত্যাগম nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        যদি (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        প্রতি (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* ধ্রুব dVtxp = edge.শীর্ষ()->as<OrderMoveVertex>();
            // Update number of dependencies
            ধ্রুব uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            যদি (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        যদি (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            প্রতি (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                যদি (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    ভঙ্গ;
                }
            }
        }

        // Finally yield the selected vertex
        প্রত্যাগম mVtxp;
    }
};

#endif  // Guard
