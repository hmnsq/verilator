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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ORDERMOVEGRAPH_H_
#وصف_ڪريو VERILATOR_V3ORDERMOVEGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3Order.h"
#ضمن "V3OrderGraph.h"

class ترتيب_Move_Dom_Scope;
class ترتيب_Move_گراف;

//======================================================================
// Vertex types

class ترتيب_Move_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(ترتيب_Move_راس, V3_گراف_راس)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    ترتيب_Logic_راس* ثابت m_logicp;
    ترتيب_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<ترتيب_Move_راس> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ثابت override { أرجع logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ثابت override VL_MT_STABLE {
        std::string nm;
        إذا (!logicp()) {
            nm = "var";
        } وإلا {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvt_منزل_Hex(logicp()->domainp());
            nm += +" s=" + cvt_منزل_Hex(logicp()->scopep());
        }
        إذا (userp()) nm += "\nu=" + cvt_منزل_Hex(userp());
        أرجع nm;
    }

    V3ListLinks<ترتيب_Move_راس>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<ترتيب_Move_راس, &ترتيب_Move_راس::links>;

    // CONSTRUCTORS
    ترتيب_Move_راس(ترتيب_Move_گراف& گراف, ترتيب_Logic_راس* lVtxp,
                    ثابت ترتيب_Domain* domainp) VL_MT_DISABLED;
    ~ترتيب_Move_راس() override = افتراضي;
    VL_UNCOPYABLE(ترتيب_Move_راس);

    ترتيب_Logic_راس* logicp() ثابت VL_MT_STABLE { أرجع m_logicp; }
    ترتيب_Move_Dom_Scope& domScope() ثابت { أرجع m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class ترتيب_Move_گراف final : عام V3_گراف {
    ترتيب_گراف& m_ترتيب_گراف;  // The OrderGraph this move graph was built from

عام:
    explicit ترتيب_Move_گراف(ترتيب_گراف& ترتيب_گراف_2)
        : m_ترتيب_گراف{ترتيب_گراف_2} {}

    ترتيب_گراف& ترتيب_گراف_2() ثابت { أرجع m_ترتيب_گراف; }

    // Build an OrderMoveGraph from an OrderGraph
    ساكن std::unique_ptr<ترتيب_Move_گراف> build(ترتيب_گراف&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class ترتيب_Move_Dom_Scope final {
    // STATE
    ترتيب_Move_راس::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<ترتيب_Move_Dom_Scope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    ثابت ترتيب_Domain* ثابت m_domainp;  // Domain the vertices belong to
    ثابت AstScope* ثابت m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_ڪنجي final {
        ثابت ترتيب_Domain* ثابت m_domainp;  // Domain the vertices belong to
        ثابت AstScope* ثابت m_scopep;  // Scope the vertices belong to

    عام:
        Dom_Scope_Map_ڪنجي(ثابت ترتيب_Domain* domainp, ثابت AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        بنية Hash final {
            سائيز_t عامل()(ثابت Dom_Scope_Map_ڪنجي& ڪنجي) ثابت {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{ڪنجي.m_domainp};
                hash += ڪنجي.m_scopep;
                أرجع hash.قدر();
            }
        };

        بنية Equal final {
            bool عامل()(ثابت Dom_Scope_Map_ڪنجي& a, ثابت Dom_Scope_Map_ڪنجي& b) ثابت {
                أرجع a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_ڪنجي, ترتيب_Move_Dom_Scope, Dom_Scope_Map_ڪنجي::Hash,
                                           Dom_Scope_Map_ڪنجي::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ساكن DomScopeMap s_dsMap;

    V3ListLinks<ترتيب_Move_Dom_Scope>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<ترتيب_Move_Dom_Scope, &ترتيب_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    ساكن ترتيب_Move_Dom_Scope& getOrCreate(ثابت ترتيب_Domain* domainp, ثابت AstScope* scopep) {
        أرجع s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .پهريون->ٻيو;
    }
    ساكن فراغ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    ترتيب_Move_Dom_Scope(ثابت ترتيب_Domain* domainp, ثابت AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~ترتيب_Move_Dom_Scope() = افتراضي;
    VL_UNCOPYABLE(ترتيب_Move_Dom_Scope);
    VL_UNMOVABLE(ترتيب_Move_Dom_Scope);

    // MEMBERS
    ترتيب_Move_راس::List& readyVertices() { أرجع m_readyVertices; }
    ثابت ترتيب_Domain* domainp() ثابت { أرجع m_domainp; }
    ثابت AstScope* scopep() ثابت { أرجع m_scopep; }

    bool isOnList() ثابت { أرجع m_isOnList; }
    فراغ isOnList(bool قدر) { m_isOnList = قدر; }
};

//======================================================================
// Serializer for OrderMoveGraph

class ترتيب_Move_گراف_Serializer final {
    // STATE
    ترتيب_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    ترتيب_Move_Dom_Scope* m_ايندڙ_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    فراغ ready(ترتيب_Move_راس* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        إذا (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            ترتيب_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_پٺ(vtxp);
            // Add the DomScope to the global ready list if not there yet
            إذا (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_پٺ(&domScope);
            }
        } وإلا {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->ٻاهريون_ڪنارا()) {
                // The dependent variable
                ترتيب_Move_راس* ثابت dVtxp = ڪنارو.چوٽي()->as<ترتيب_Move_راس>();
                // Update number of dependencies
                ثابت uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                إذا (!nDeps) ready(dVtxp);
            }
        }
    }

عام:
    // CONSTRUCTOR
    explicit ترتيب_Move_گراف_Serializer(ترتيب_Move_گراف& move_گراف) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        لكل (V3_گراف_راس& vtx : move_گراف.vertices()) {
            ثابت uint32_t nDeps = vtx.اندريون_ڪنارا().سائيز();
            vtx.user(nDeps);
        }
    }
    ~ترتيب_Move_گراف_Serializer() = افتراضي;
    VL_UNCOPYABLE(ترتيب_Move_گراف_Serializer);

    // Add a seed vertex to the ready lists
    فراغ جوڙيو_ٻج(ترتيب_Move_راس* vtxp) { ready(vtxp); }

    ترتيب_Move_راس* get_ايندڙ() {
        إذا (!m_ايندڙ_Dom_Scopep) m_ايندڙ_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        إذا (!m_ايندڙ_Dom_Scopep) أرجع nullptr;
        ترتيب_Move_Dom_Scope& currDomScope = *m_ايندڙ_Dom_Scopep;

        ترتيب_Move_راس::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.خالي_جايون(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        ترتيب_Move_راس* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        إذا (currReadyList.خالي_جايون()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        لكل (V3_گراف_ڪنارو& ڪنارو : mVtxp->ٻاهريون_ڪنارا()) {
            // The dependent variable
            ترتيب_Move_راس* ثابت dVtxp = ڪنارو.چوٽي()->as<ترتيب_Move_راس>();
            // Update number of dependencies
            ثابت uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            إذا (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        إذا (currReadyList.خالي_جايون()) {
            m_ايندڙ_Dom_Scopep = nullptr;
            لكل (ترتيب_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                إذا (domScope.domainp() == currDomScope.domainp()) {
                    m_ايندڙ_Dom_Scopep = &domScope;
                    اكسر;
                }
            }
        }

        // Finally yield the selected vertex
        أرجع mVtxp;
    }
};

#جيڪڏهن_پورو  // Guard
