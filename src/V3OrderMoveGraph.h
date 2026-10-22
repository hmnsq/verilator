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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* स्थिरांक m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() स्थिरांक override { परत logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() स्थिरांक override VL_MT_STABLE {
        std::string nm;
        जर (!logicp()) {
            nm = "var";
        } अन्यथा {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        जर (userp()) nm += "\nu=" + cvtToHex(userp());
        परत nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { परत m_links; }

सार्वजनिक:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    स्थिरांक OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = पूर्वनिर्धारित;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() स्थिरांक VL_MT_STABLE { परत m_logicp; }
    OrderMoveDomScope& domScope() स्थिरांक { परत m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : सार्वजनिक V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

सार्वजनिक:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() स्थिरांक { परत m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    स्थिर std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    स्थिरांक OrderDomain* स्थिरांक m_domainp;  // Domain the vertices belong to
    स्थिरांक AstScope* स्थिरांक m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_किल्ली final {
        स्थिरांक OrderDomain* स्थिरांक m_domainp;  // Domain the vertices belong to
        स्थिरांक AstScope* स्थिरांक m_scopep;  // Scope the vertices belong to

    सार्वजनिक:
        Dom_Scope_Map_किल्ली(स्थिरांक OrderDomain* domainp, स्थिरांक AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        संरचना Hash final {
            माप_प्रकार चालक()(स्थिरांक Dom_Scope_Map_किल्ली& किल्ली) स्थिरांक {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{किल्ली.m_domainp};
                hash += किल्ली.m_scopep;
                परत hash.मूल्य();
            }
        };

        संरचना Equal final {
            bool चालक()(स्थिरांक Dom_Scope_Map_किल्ली& a, स्थिरांक Dom_Scope_Map_किल्ली& b) स्थिरांक {
                परत a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_किल्ली, OrderMoveDomScope, Dom_Scope_Map_किल्ली::Hash,
                                           Dom_Scope_Map_किल्ली::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    स्थिर DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { परत m_links; }

सार्वजनिक:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    स्थिर OrderMoveDomScope& getOrCreate(स्थिरांक OrderDomain* domainp, स्थिरांक AstScope* scopep) {
        परत s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    स्थिर शून्य clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(स्थिरांक OrderDomain* domainp, स्थिरांक AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { परत m_readyVertices; }
    स्थिरांक OrderDomain* domainp() स्थिरांक { परत m_domainp; }
    स्थिरांक AstScope* scopep() स्थिरांक { परत m_scopep; }

    bool isOnList() स्थिरांक { परत m_isOnList; }
    शून्य isOnList(bool मूल्य) { m_isOnList = मूल्य; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_nextDomScopep = nullptr;  // Next DomScope to yield from

    // METHODS

    शून्य ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        जर (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            जर (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } अन्यथा {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            साठी (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* स्थिरांक dVtxp = edge.शीर्ष()->as<OrderMoveVertex>();
                // Update number of dependencies
                स्थिरांक uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                जर (!nDeps) ready(dVtxp);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        साठी (V3GraphVertex& vtx : moveGraph.vertices()) {
            स्थिरांक uint32_t nDeps = vtx.inEdges().size();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = पूर्वनिर्धारित;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    शून्य addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* getNext() {
        जर (!m_nextDomScopep) m_nextDomScopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        जर (!m_nextDomScopep) परत nullptr;
        OrderMoveDomScope& currDomScope = *m_nextDomScopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        जर (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        साठी (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* स्थिरांक dVtxp = edge.शीर्ष()->as<OrderMoveVertex>();
            // Update number of dependencies
            स्थिरांक uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            जर (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        जर (currReadyList.empty()) {
            m_nextDomScopep = nullptr;
            साठी (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                जर (domScope.domainp() == currDomScope.domainp()) {
                    m_nextDomScopep = &domScope;
                    थांबा;
                }
            }
        }

        // Finally yield the selected vertex
        परत mVtxp;
    }
};

#endif  // Guard
