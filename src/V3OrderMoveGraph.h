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

#अगर_अघोषित VERILATOR_V3ORDERMOVEGRAPH_H_
#घोषणा VERILATOR_V3ORDERMOVEGRAPH_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Graph.h"
#समावेश "V3Order.h"
#समावेश "V3OrderGraph.h"

class OrderMoveDomScope;
class OrderMoveGraph;

//======================================================================
// Vertex types

class OrderMoveVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(OrderMoveVertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    OrderLogicVertex* स्थिर m_logicp;
    OrderMoveDomScope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<OrderMoveVertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() स्थिर override { वापस logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() स्थिर override VL_MT_STABLE {
        std::string nm;
        अगर (!logicp()) {
            nm = "var";
        } अन्यथा {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvtToHex(logicp()->domainp());
            nm += +" s=" + cvtToHex(logicp()->scopep());
        }
        अगर (userp()) nm += "\nu=" + cvtToHex(userp());
        वापस nm;
    }

    V3ListLinks<OrderMoveVertex>& links() { वापस m_links; }

खुला:
    // List type to store instances of this class
    using List = V3List<OrderMoveVertex, &OrderMoveVertex::links>;

    // CONSTRUCTORS
    OrderMoveVertex(OrderMoveGraph& graph, OrderLogicVertex* lVtxp,
                    स्थिर OrderDomain* domainp) VL_MT_DISABLED;
    ~OrderMoveVertex() override = शेष;
    VL_UNCOPYABLE(OrderMoveVertex);

    OrderLogicVertex* logicp() स्थिर VL_MT_STABLE { वापस m_logicp; }
    OrderMoveDomScope& domScope() स्थिर { वापस m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class OrderMoveGraph final : खुला V3Graph {
    OrderGraph& m_orderGraph;  // The OrderGraph this move graph was built from

खुला:
    explicit OrderMoveGraph(OrderGraph& orderGraph)
        : m_orderGraph{orderGraph} {}

    OrderGraph& orderGraph() स्थिर { वापस m_orderGraph; }

    // Build an OrderMoveGraph from an OrderGraph
    अटल std::unique_ptr<OrderMoveGraph> build(OrderGraph&);
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
    स्थिर OrderDomain* स्थिर m_domainp;  // Domain the vertices belong to
    स्थिर AstScope* स्थिर m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_कुंजी final {
        स्थिर OrderDomain* स्थिर m_domainp;  // Domain the vertices belong to
        स्थिर AstScope* स्थिर m_scopep;  // Scope the vertices belong to

    खुला:
        Dom_Scope_Map_कुंजी(स्थिर OrderDomain* domainp, स्थिर AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        काष्ठा Hash final {
            माप_प्रकार चालक()(स्थिर Dom_Scope_Map_कुंजी& कुंजी) स्थिर {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{कुंजी.m_domainp};
                hash += कुंजी.m_scopep;
                वापस hash.मूल्य();
            }
        };

        काष्ठा Equal final {
            bool चालक()(स्थिर Dom_Scope_Map_कुंजी& a, स्थिर Dom_Scope_Map_कुंजी& b) स्थिर {
                वापस a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_कुंजी, OrderMoveDomScope, Dom_Scope_Map_कुंजी::Hash,
                                           Dom_Scope_Map_कुंजी::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    अटल DomScopeMap s_dsMap;

    V3ListLinks<OrderMoveDomScope>& links() { वापस m_links; }

खुला:
    // List type to store instances of this class
    using List = V3List<OrderMoveDomScope, &OrderMoveDomScope::links>;

    // STATIC MEMBERS
    अटल OrderMoveDomScope& getOrCreate(स्थिर OrderDomain* domainp, स्थिर AstScope* scopep) {
        वापस s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    अटल व्योम clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    OrderMoveDomScope(स्थिर OrderDomain* domainp, स्थिर AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~OrderMoveDomScope() = शेष;
    VL_UNCOPYABLE(OrderMoveDomScope);
    VL_UNMOVABLE(OrderMoveDomScope);

    // MEMBERS
    OrderMoveVertex::List& readyVertices() { वापस m_readyVertices; }
    स्थिर OrderDomain* domainp() स्थिर { वापस m_domainp; }
    स्थिर AstScope* scopep() स्थिर { वापस m_scopep; }

    bool isOnList() स्थिर { वापस m_isOnList; }
    व्योम isOnList(bool मूल्य) { m_isOnList = मूल्य; }
};

//======================================================================
// Serializer for OrderMoveGraph

class OrderMoveGraphSerializer final {
    // STATE
    OrderMoveDomScope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    OrderMoveDomScope* m_अग्र_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    व्योम ready(OrderMoveVertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        अगर (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            OrderMoveDomScope& domScope = vtxp->domScope();
            domScope.readyVertices().linkBack(vtxp);
            // Add the DomScope to the global ready list if not there yet
            अगर (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.linkBack(&domScope);
            }
        } अन्यथा {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            क्रम (V3GraphEdge& edge : vtxp->outEdges()) {
                // The dependent variable
                OrderMoveVertex* स्थिर dVtxp = edge.शीर्ष()->as<OrderMoveVertex>();
                // Update number of dependencies
                स्थिर uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                अगर (!nDeps) ready(dVtxp);
            }
        }
    }

खुला:
    // CONSTRUCTOR
    explicit OrderMoveGraphSerializer(OrderMoveGraph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        क्रम (V3GraphVertex& vtx : moveGraph.vertices()) {
            स्थिर uint32_t nDeps = vtx.inEdges().आकार();
            vtx.user(nDeps);
        }
    }
    ~OrderMoveGraphSerializer() = शेष;
    VL_UNCOPYABLE(OrderMoveGraphSerializer);

    // Add a seed vertex to the ready lists
    व्योम addSeed(OrderMoveVertex* vtxp) { ready(vtxp); }

    OrderMoveVertex* get_अग्र() {
        अगर (!m_अग्र_Dom_Scopep) m_अग्र_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        अगर (!m_अग्र_Dom_Scopep) वापस nullptr;
        OrderMoveDomScope& currDomScope = *m_अग्र_Dom_Scopep;

        OrderMoveVertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.empty(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        OrderMoveVertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        अगर (currReadyList.empty()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        क्रम (V3GraphEdge& edge : mVtxp->outEdges()) {
            // The dependent variable
            OrderMoveVertex* स्थिर dVtxp = edge.शीर्ष()->as<OrderMoveVertex>();
            // Update number of dependencies
            स्थिर uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            अगर (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        अगर (currReadyList.empty()) {
            m_अग्र_Dom_Scopep = nullptr;
            क्रम (OrderMoveDomScope& domScope : m_readyDomScopeps) {
                अगर (domScope.domainp() == currDomScope.domainp()) {
                    m_अग्र_Dom_Scopep = &domScope;
                    अवरोध;
                }
            }
        }

        // Finally yield the selected vertex
        वापस mVtxp;
    }
};

#पूर्ण_अगर  // Guard
