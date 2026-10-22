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

#यदि_अपरिभाषितम् VERILATOR_V3ORDERMOVEGRAPH_H_
#परिभाषय VERILATOR_V3ORDERMOVEGRAPH_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3OrderGraph.h"

class क्रमः_Move_Dom_Scope;
class क्रमः_Move_Graph;

//======================================================================
// Vertex types

class क्रमः_Move_Vertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(क्रमः_Move_Vertex, V3GraphVertex)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    क्रमः_Logic_Vertex* ध्रुव m_logicp;
    क्रमः_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<क्रमः_Move_Vertex> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ध्रुव override { प्रत्यागम logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ध्रुव override VL_MT_STABLE {
        std::string nm;
        यदि (!logicp()) {
            nm = "var";
        } अन्यथा {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvt_गन्तव्यम्_Hex(logicp()->domainp());
            nm += +" s=" + cvt_गन्तव्यम्_Hex(logicp()->scopep());
        }
        यदि (userp()) nm += "\nu=" + cvt_गन्तव्यम्_Hex(userp());
        प्रत्यागम nm;
    }

    V3ListLinks<क्रमः_Move_Vertex>& links() { प्रत्यागम m_links; }

सार्वजनिक:
    // List type to store instances of this class
    using List = V3List<क्रमः_Move_Vertex, &क्रमः_Move_Vertex::links>;

    // CONSTRUCTORS
    क्रमः_Move_Vertex(क्रमः_Move_Graph& graph, क्रमः_Logic_Vertex* lVtxp,
                    ध्रुव क्रमः_Domain* domainp) VL_MT_DISABLED;
    ~क्रमः_Move_Vertex() override = स्वभाव;
    VL_UNCOPYABLE(क्रमः_Move_Vertex);

    क्रमः_Logic_Vertex* logicp() ध्रुव VL_MT_STABLE { प्रत्यागम m_logicp; }
    क्रमः_Move_Dom_Scope& domScope() ध्रुव { प्रत्यागम m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class क्रमः_Move_Graph final : सार्वजनिक V3Graph {
    क्रमः_Graph& m_क्रमः_Graph;  // The OrderGraph this move graph was built from

सार्वजनिक:
    explicit क्रमः_Move_Graph(क्रमः_Graph& क्रमः_Graph_2)
        : m_क्रमः_Graph{क्रमः_Graph_2} {}

    क्रमः_Graph& क्रमः_Graph_2() ध्रुव { प्रत्यागम m_क्रमः_Graph; }

    // Build an OrderMoveGraph from an OrderGraph
    स्थिर std::unique_ptr<क्रमः_Move_Graph> build(क्रमः_Graph&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class क्रमः_Move_Dom_Scope final {
    // STATE
    क्रमः_Move_Vertex::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<क्रमः_Move_Dom_Scope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    ध्रुव क्रमः_Domain* ध्रुव m_domainp;  // Domain the vertices belong to
    ध्रुव AstScope* ध्रुव m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_कुञ्चिका final {
        ध्रुव क्रमः_Domain* ध्रुव m_domainp;  // Domain the vertices belong to
        ध्रुव AstScope* ध्रुव m_scopep;  // Scope the vertices belong to

    सार्वजनिक:
        Dom_Scope_Map_कुञ्चिका(ध्रुव क्रमः_Domain* domainp, ध्रुव AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        संरचना Hash final {
            आकार_t संक्रियकः()(ध्रुव Dom_Scope_Map_कुञ्चिका& कुञ्चिका) ध्रुव {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{कुञ्चिका.m_domainp};
                hash += कुञ्चिका.m_scopep;
                प्रत्यागम hash.मानम्();
            }
        };

        संरचना Equal final {
            bool संक्रियकः()(ध्रुव Dom_Scope_Map_कुञ्चिका& a, ध्रुव Dom_Scope_Map_कुञ्चिका& b) ध्रुव {
                प्रत्यागम a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_कुञ्चिका, क्रमः_Move_Dom_Scope, Dom_Scope_Map_कुञ्चिका::Hash,
                                           Dom_Scope_Map_कुञ्चिका::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    स्थिर DomScopeMap s_dsMap;

    V3ListLinks<क्रमः_Move_Dom_Scope>& links() { प्रत्यागम m_links; }

सार्वजनिक:
    // List type to store instances of this class
    using List = V3List<क्रमः_Move_Dom_Scope, &क्रमः_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    स्थिर क्रमः_Move_Dom_Scope& getOrCreate(ध्रुव क्रमः_Domain* domainp, ध्रुव AstScope* scopep) {
        प्रत्यागम s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .first->second;
    }
    स्थिर शून्य clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    क्रमः_Move_Dom_Scope(ध्रुव क्रमः_Domain* domainp, ध्रुव AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~क्रमः_Move_Dom_Scope() = स्वभाव;
    VL_UNCOPYABLE(क्रमः_Move_Dom_Scope);
    VL_UNMOVABLE(क्रमः_Move_Dom_Scope);

    // MEMBERS
    क्रमः_Move_Vertex::List& readyVertices() { प्रत्यागम m_readyVertices; }
    ध्रुव क्रमः_Domain* domainp() ध्रुव { प्रत्यागम m_domainp; }
    ध्रुव AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }

    bool isOnList() ध्रुव { प्रत्यागम m_isOnList; }
    शून्य isOnList(bool मानम्) { m_isOnList = मानम्; }
};

//======================================================================
// Serializer for OrderMoveGraph

class क्रमः_Move_Graph_Serializer final {
    // STATE
    क्रमः_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    क्रमः_Move_Dom_Scope* m_अग्र_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    शून्य ready(क्रमः_Move_Vertex* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        यदि (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            क्रमः_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_पृष्ठभागः(vtxp);
            // Add the DomScope to the global ready list if not there yet
            यदि (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_पृष्ठभागः(&domScope);
            }
        } अन्यथा {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            प्रति (V3GraphEdge& edge : vtxp->बहिर्गमनम्_धाराः()) {
                // The dependent variable
                क्रमः_Move_Vertex* ध्रुव dVtxp = edge.शिखरम्()->as<क्रमः_Move_Vertex>();
                // Update number of dependencies
                ध्रुव uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                यदि (!nDeps) ready(dVtxp);
            }
        }
    }

सार्वजनिक:
    // CONSTRUCTOR
    explicit क्रमः_Move_Graph_Serializer(क्रमः_Move_Graph& moveGraph) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        प्रति (V3GraphVertex& vtx : moveGraph.vertices()) {
            ध्रुव uint32_t nDeps = vtx.अन्तर्_धाराः().आकार();
            vtx.user(nDeps);
        }
    }
    ~क्रमः_Move_Graph_Serializer() = स्वभाव;
    VL_UNCOPYABLE(क्रमः_Move_Graph_Serializer);

    // Add a seed vertex to the ready lists
    शून्य सङ्कलय_बीजम्(क्रमः_Move_Vertex* vtxp) { ready(vtxp); }

    क्रमः_Move_Vertex* get_अग्र() {
        यदि (!m_अग्र_Dom_Scopep) m_अग्र_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        यदि (!m_अग्र_Dom_Scopep) प्रत्यागम nullptr;
        क्रमः_Move_Dom_Scope& currDomScope = *m_अग्र_Dom_Scopep;

        क्रमः_Move_Vertex::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.रिक्तानि(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        क्रमः_Move_Vertex* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        यदि (currReadyList.रिक्तानि()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        प्रति (V3GraphEdge& edge : mVtxp->बहिर्गमनम्_धाराः()) {
            // The dependent variable
            क्रमः_Move_Vertex* ध्रुव dVtxp = edge.शिखरम्()->as<क्रमः_Move_Vertex>();
            // Update number of dependencies
            ध्रुव uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            यदि (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        यदि (currReadyList.रिक्तानि()) {
            m_अग्र_Dom_Scopep = nullptr;
            प्रति (क्रमः_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                यदि (domScope.domainp() == currDomScope.domainp()) {
                    m_अग्र_Dom_Scopep = &domScope;
                    भङ्ग;
                }
            }
        }

        // Finally yield the selected vertex
        प्रत्यागम mVtxp;
    }
};

#यदि_अन्तः  // Guard
