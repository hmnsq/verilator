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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ORDERMOVEGRAPH_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ORDERMOVEGRAPH_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Order.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderGraph.h"

class ਕ੍ਰਮ_Move_Dom_Scope;
class ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ;

//======================================================================
// Vertex types

class ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ m_logicp;
    ਕ੍ਰਮ_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ਅਚਲ override VL_MT_STABLE {
        std::string nm;
        ਜੇ (!logicp()) {
            nm = "var";
        } ਨਹੀਂਤਾਂ {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvt_ਮੰਜ਼ਿਲ_Hex(logicp()->domainp());
            nm += +" s=" + cvt_ਮੰਜ਼ਿਲ_Hex(logicp()->scopep());
        }
        ਜੇ (userp()) nm += "\nu=" + cvt_ਮੰਜ਼ਿਲ_Hex(userp());
        ਵਾਪਸ nm;
    }

    V3ListLinks<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>& links() { ਵਾਪਸ m_links; }

ਜਨਤਕ:
    // List type to store instances of this class
    using List = V3List<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼, &ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::links>;

    // CONSTRUCTORS
    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* lVtxp,
                    ਅਚਲ ਕ੍ਰਮ_Domain* domainp) VL_MT_DISABLED;
    ~ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    VL_UNCOPYABLE(ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼);

    ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* logicp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_logicp; }
    ਕ੍ਰਮ_Move_Dom_Scope& domScope() ਅਚਲ { ਵਾਪਸ m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    ਕ੍ਰਮ_ਗ੍ਰਾਫ& m_ਕ੍ਰਮ_ਗ੍ਰਾਫ;  // The OrderGraph this move graph was built from

ਜਨਤਕ:
    explicit ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਕ੍ਰਮ_ਗ੍ਰਾਫ_2)
        : m_ਕ੍ਰਮ_ਗ੍ਰਾਫ{ਕ੍ਰਮ_ਗ੍ਰਾਫ_2} {}

    ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਕ੍ਰਮ_ਗ੍ਰਾਫ_2() ਅਚਲ { ਵਾਪਸ m_ਕ੍ਰਮ_ਗ੍ਰਾਫ; }

    // Build an OrderMoveGraph from an OrderGraph
    ਸਥਿਰ std::unique_ptr<ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ> build(ਕ੍ਰਮ_ਗ੍ਰਾਫ&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class ਕ੍ਰਮ_Move_Dom_Scope final {
    // STATE
    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<ਕ੍ਰਮ_Move_Dom_Scope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ m_domainp;  // Domain the vertices belong to
    ਅਚਲ AstScope* ਅਚਲ m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_ਕੁੰਜੀ final {
        ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ m_domainp;  // Domain the vertices belong to
        ਅਚਲ AstScope* ਅਚਲ m_scopep;  // Scope the vertices belong to

    ਜਨਤਕ:
        Dom_Scope_Map_ਕੁੰਜੀ(ਅਚਲ ਕ੍ਰਮ_Domain* domainp, ਅਚਲ AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        ਬਣਤਰ Hash final {
            ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dom_Scope_Map_ਕੁੰਜੀ& ਕੁੰਜੀ) ਅਚਲ {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{ਕੁੰਜੀ.m_domainp};
                hash += ਕੁੰਜੀ.m_scopep;
                ਵਾਪਸ hash.ਮੁੱਲ();
            }
        };

        ਬਣਤਰ Equal final {
            bool ਸੰਚਾਲਕ()(ਅਚਲ Dom_Scope_Map_ਕੁੰਜੀ& a, ਅਚਲ Dom_Scope_Map_ਕੁੰਜੀ& b) ਅਚਲ {
                ਵਾਪਸ a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_ਕੁੰਜੀ, ਕ੍ਰਮ_Move_Dom_Scope, Dom_Scope_Map_ਕੁੰਜੀ::Hash,
                                           Dom_Scope_Map_ਕੁੰਜੀ::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ਸਥਿਰ DomScopeMap s_dsMap;

    V3ListLinks<ਕ੍ਰਮ_Move_Dom_Scope>& links() { ਵਾਪਸ m_links; }

ਜਨਤਕ:
    // List type to store instances of this class
    using List = V3List<ਕ੍ਰਮ_Move_Dom_Scope, &ਕ੍ਰਮ_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    ਸਥਿਰ ਕ੍ਰਮ_Move_Dom_Scope& getOrCreate(ਅਚਲ ਕ੍ਰਮ_Domain* domainp, ਅਚਲ AstScope* scopep) {
        ਵਾਪਸ s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .ਪਹਿਲਾ->ਦੂਜਾ;
    }
    ਸਥਿਰ ਖਾਲੀ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    ਕ੍ਰਮ_Move_Dom_Scope(ਅਚਲ ਕ੍ਰਮ_Domain* domainp, ਅਚਲ AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~ਕ੍ਰਮ_Move_Dom_Scope() = ਮੂਲ;
    VL_UNCOPYABLE(ਕ੍ਰਮ_Move_Dom_Scope);
    VL_UNMOVABLE(ਕ੍ਰਮ_Move_Dom_Scope);

    // MEMBERS
    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List& readyVertices() { ਵਾਪਸ m_readyVertices; }
    ਅਚਲ ਕ੍ਰਮ_Domain* domainp() ਅਚਲ { ਵਾਪਸ m_domainp; }
    ਅਚਲ AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }

    bool isOnList() ਅਚਲ { ਵਾਪਸ m_isOnList; }
    ਖਾਲੀ isOnList(bool ਮੁੱਲ) { m_isOnList = ਮੁੱਲ; }
};

//======================================================================
// Serializer for OrderMoveGraph

class ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Serializer final {
    // STATE
    ਕ੍ਰਮ_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    ਕ੍ਰਮ_Move_Dom_Scope* m_ਅਗਲਾ_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    ਖਾਲੀ ready(ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        ਜੇ (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            ਕ੍ਰਮ_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_ਪਿੱਠ(vtxp);
            // Add the DomScope to the global ready list if not there yet
            ਜੇ (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_ਪਿੱਠ(&domScope);
            }
        } ਨਹੀਂਤਾਂ {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                // The dependent variable
                ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ dVtxp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>();
                // Update number of dependencies
                ਅਚਲ uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                ਜੇ (!nDeps) ready(dVtxp);
            }
        }
    }

ਜਨਤਕ:
    // CONSTRUCTOR
    explicit ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Serializer(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : move_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ uint32_t nDeps = vtx.ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ();
            vtx.user(nDeps);
        }
    }
    ~ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Serializer() = ਮੂਲ;
    VL_UNCOPYABLE(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Serializer);

    // Add a seed vertex to the ready lists
    ਖਾਲੀ ਜੋੜੋ_ਬੀਜ(ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* vtxp) { ready(vtxp); }

    ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* get_ਅਗਲਾ() {
        ਜੇ (!m_ਅਗਲਾ_Dom_Scopep) m_ਅਗਲਾ_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        ਜੇ (!m_ਅਗਲਾ_Dom_Scopep) ਵਾਪਸ nullptr;
        ਕ੍ਰਮ_Move_Dom_Scope& currDomScope = *m_ਅਗਲਾ_Dom_Scopep;

        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.ਖਾਲੀ_ਥਾਵਾਂ(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        ਜੇ (currReadyList.ਖਾਲੀ_ਥਾਵਾਂ()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            // The dependent variable
            ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ dVtxp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>();
            // Update number of dependencies
            ਅਚਲ uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            ਜੇ (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        ਜੇ (currReadyList.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_ਅਗਲਾ_Dom_Scopep = nullptr;
            ਲਈ (ਕ੍ਰਮ_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                ਜੇ (domScope.domainp() == currDomScope.domainp()) {
                    m_ਅਗਲਾ_Dom_Scopep = &domScope;
                    ਰੋਕੋ;
                }
            }
        }

        // Finally yield the selected vertex
        ਵਾਪਸ mVtxp;
    }
};

#ਜੇ_ਅੰਤ  // Guard
