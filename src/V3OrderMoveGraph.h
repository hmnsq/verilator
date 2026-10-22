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

#నిర్వచితం_కాకపోతే VERILATOR_V3ORDERMOVEGRAPH_H_
#నిర్వచించు VERILATOR_V3ORDERMOVEGRAPH_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Order.h"
#అన్తర్భావ "V3OrderGraph.h"

class క్రమం_Move_Dom_Scope;
class క్రమం_Move_లేఖాచిత్రం;

//======================================================================
// Vertex types

class క్రమం_Move_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Move_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    క్రమం_Logic_శీర్షం* స్థిరరాశి m_logicp;
    క్రమం_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<క్రమం_Move_శీర్షం> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() స్థిరరాశి override { తిరిగి logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() స్థిరరాశి override VL_MT_STABLE {
        std::string nm;
        ఒకవేళ (!logicp()) {
            nm = "var";
        } లేకపోతే {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvt_గమ్యం_Hex(logicp()->domainp());
            nm += +" s=" + cvt_గమ్యం_Hex(logicp()->scopep());
        }
        ఒకవేళ (userp()) nm += "\nu=" + cvt_గమ్యం_Hex(userp());
        తిరిగి nm;
    }

    V3ListLinks<క్రమం_Move_శీర్షం>& links() { తిరిగి m_links; }

బహిరంగ:
    // List type to store instances of this class
    using List = V3List<క్రమం_Move_శీర్షం, &క్రమం_Move_శీర్షం::links>;

    // CONSTRUCTORS
    క్రమం_Move_శీర్షం(క్రమం_Move_లేఖాచిత్రం& లేఖాచిత్రం, క్రమం_Logic_శీర్షం* lVtxp,
                    స్థిరరాశి క్రమం_Domain* domainp) VL_MT_DISABLED;
    ~క్రమం_Move_శీర్షం() override = అప్రమేయ;
    VL_UNCOPYABLE(క్రమం_Move_శీర్షం);

    క్రమం_Logic_శీర్షం* logicp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_logicp; }
    క్రమం_Move_Dom_Scope& domScope() స్థిరరాశి { తిరిగి m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class క్రమం_Move_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    క్రమం_లేఖాచిత్రం& m_క్రమం_లేఖాచిత్రం;  // The OrderGraph this move graph was built from

బహిరంగ:
    explicit క్రమం_Move_లేఖాచిత్రం(క్రమం_లేఖాచిత్రం& క్రమం_లేఖాచిత్రం_2)
        : m_క్రమం_లేఖాచిత్రం{క్రమం_లేఖాచిత్రం_2} {}

    క్రమం_లేఖాచిత్రం& క్రమం_లేఖాచిత్రం_2() స్థిరరాశి { తిరిగి m_క్రమం_లేఖాచిత్రం; }

    // Build an OrderMoveGraph from an OrderGraph
    స్థిర std::unique_ptr<క్రమం_Move_లేఖాచిత్రం> build(క్రమం_లేఖాచిత్రం&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class క్రమం_Move_Dom_Scope final {
    // STATE
    క్రమం_Move_శీర్షం::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<క్రమం_Move_Dom_Scope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    స్థిరరాశి క్రమం_Domain* స్థిరరాశి m_domainp;  // Domain the vertices belong to
    స్థిరరాశి AstScope* స్థిరరాశి m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_కీలకం final {
        స్థిరరాశి క్రమం_Domain* స్థిరరాశి m_domainp;  // Domain the vertices belong to
        స్థిరరాశి AstScope* స్థిరరాశి m_scopep;  // Scope the vertices belong to

    బహిరంగ:
        Dom_Scope_Map_కీలకం(స్థిరరాశి క్రమం_Domain* domainp, స్థిరరాశి AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        నిర్మాణం Hash final {
            కొలత_t క్రియాకారకం()(స్థిరరాశి Dom_Scope_Map_కీలకం& కీలకం) స్థిరరాశి {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{కీలకం.m_domainp};
                hash += కీలకం.m_scopep;
                తిరిగి hash.విలువ();
            }
        };

        నిర్మాణం Equal final {
            bool క్రియాకారకం()(స్థిరరాశి Dom_Scope_Map_కీలకం& a, స్థిరరాశి Dom_Scope_Map_కీలకం& b) స్థిరరాశి {
                తిరిగి a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_కీలకం, క్రమం_Move_Dom_Scope, Dom_Scope_Map_కీలకం::Hash,
                                           Dom_Scope_Map_కీలకం::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    స్థిర DomScopeMap s_dsMap;

    V3ListLinks<క్రమం_Move_Dom_Scope>& links() { తిరిగి m_links; }

బహిరంగ:
    // List type to store instances of this class
    using List = V3List<క్రమం_Move_Dom_Scope, &క్రమం_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    స్థిర క్రమం_Move_Dom_Scope& getOrCreate(స్థిరరాశి క్రమం_Domain* domainp, స్థిరరాశి AstScope* scopep) {
        తిరిగి s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .మొదటి->రెండవ;
    }
    స్థిర శూన్య clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    క్రమం_Move_Dom_Scope(స్థిరరాశి క్రమం_Domain* domainp, స్థిరరాశి AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~క్రమం_Move_Dom_Scope() = అప్రమేయ;
    VL_UNCOPYABLE(క్రమం_Move_Dom_Scope);
    VL_UNMOVABLE(క్రమం_Move_Dom_Scope);

    // MEMBERS
    క్రమం_Move_శీర్షం::List& readyVertices() { తిరిగి m_readyVertices; }
    స్థిరరాశి క్రమం_Domain* domainp() స్థిరరాశి { తిరిగి m_domainp; }
    స్థిరరాశి AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }

    bool isOnList() స్థిరరాశి { తిరిగి m_isOnList; }
    శూన్య isOnList(bool విలువ) { m_isOnList = విలువ; }
};

//======================================================================
// Serializer for OrderMoveGraph

class క్రమం_Move_లేఖాచిత్రం_Serializer final {
    // STATE
    క్రమం_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    క్రమం_Move_Dom_Scope* m_తదుపరి_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    శూన్య ready(క్రమం_Move_శీర్షం* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        ఒకవేళ (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            క్రమం_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_వెనుక(vtxp);
            // Add the DomScope to the global ready list if not there yet
            ఒకవేళ (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_వెనుక(&domScope);
            }
        } లేకపోతే {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->బహిర్గతం_అంచులు()) {
                // The dependent variable
                క్రమం_Move_శీర్షం* స్థిరరాశి dVtxp = అంచు.శిఖరం()->as<క్రమం_Move_శీర్షం>();
                // Update number of dependencies
                స్థిరరాశి uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                ఒకవేళ (!nDeps) ready(dVtxp);
            }
        }
    }

బహిరంగ:
    // CONSTRUCTOR
    explicit క్రమం_Move_లేఖాచిత్రం_Serializer(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : move_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి uint32_t nDeps = vtx.లోపల_అంచులు().కొలత();
            vtx.user(nDeps);
        }
    }
    ~క్రమం_Move_లేఖాచిత్రం_Serializer() = అప్రమేయ;
    VL_UNCOPYABLE(క్రమం_Move_లేఖాచిత్రం_Serializer);

    // Add a seed vertex to the ready lists
    శూన్య కూడు_విత్తనం(క్రమం_Move_శీర్షం* vtxp) { ready(vtxp); }

    క్రమం_Move_శీర్షం* get_తదుపరి() {
        ఒకవేళ (!m_తదుపరి_Dom_Scopep) m_తదుపరి_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        ఒకవేళ (!m_తదుపరి_Dom_Scopep) తిరిగి nullptr;
        క్రమం_Move_Dom_Scope& currDomScope = *m_తదుపరి_Dom_Scopep;

        క్రమం_Move_శీర్షం::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.ఖాళీలు(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        క్రమం_Move_శీర్షం* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        ఒకవేళ (currReadyList.ఖాళీలు()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : mVtxp->బహిర్గతం_అంచులు()) {
            // The dependent variable
            క్రమం_Move_శీర్షం* స్థిరరాశి dVtxp = అంచు.శిఖరం()->as<క్రమం_Move_శీర్షం>();
            // Update number of dependencies
            స్థిరరాశి uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            ఒకవేళ (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        ఒకవేళ (currReadyList.ఖాళీలు()) {
            m_తదుపరి_Dom_Scopep = nullptr;
            కొరకు (క్రమం_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                ఒకవేళ (domScope.domainp() == currDomScope.domainp()) {
                    m_తదుపరి_Dom_Scopep = &domScope;
                    ఆపు;
                }
            }
        }

        // Finally yield the selected vertex
        తిరిగి mVtxp;
    }
};

#అయితే_ముగింపు  // Guard
