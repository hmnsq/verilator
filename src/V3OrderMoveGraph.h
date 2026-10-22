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

#جے_بنا_تعریف VERILATOR_V3ORDERMOVEGRAPH_H_
#تعریف_کرو VERILATOR_V3ORDERMOVEGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3Order.h"
#ضمن "V3OrderGraph.h"

class ترتیب_Move_Dom_Scope;
class ترتیب_Move_گراف;

//======================================================================
// Vertex types

class ترتیب_Move_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(ترتیب_Move_راس, V3_گراف_راس)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    ترتیب_Logic_راس* ثابت m_logicp;
    ترتیب_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<ترتیب_Move_راس> m_links;  // List links to store instances of this class

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

    V3ListLinks<ترتیب_Move_راس>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<ترتیب_Move_راس, &ترتیب_Move_راس::links>;

    // CONSTRUCTORS
    ترتیب_Move_راس(ترتیب_Move_گراف& گراف, ترتیب_Logic_راس* lVtxp,
                    ثابت ترتیب_Domain* domainp) VL_MT_DISABLED;
    ~ترتیب_Move_راس() override = افتراضي;
    VL_UNCOPYABLE(ترتیب_Move_راس);

    ترتیب_Logic_راس* logicp() ثابت VL_MT_STABLE { أرجع m_logicp; }
    ترتیب_Move_Dom_Scope& domScope() ثابت { أرجع m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class ترتیب_Move_گراف final : عام V3_گراف {
    ترتیب_گراف& m_ترتیب_گراف;  // The OrderGraph this move graph was built from

عام:
    explicit ترتیب_Move_گراف(ترتیب_گراف& ترتیب_گراف_2)
        : m_ترتیب_گراف{ترتیب_گراف_2} {}

    ترتیب_گراف& ترتیب_گراف_2() ثابت { أرجع m_ترتیب_گراف; }

    // Build an OrderMoveGraph from an OrderGraph
    ساكن std::unique_ptr<ترتیب_Move_گراف> build(ترتیب_گراف&);
};

//======================================================================
// OrderMoveDomScope

// Information stored for each unique (domain, scope) pair. Mainly a list of ready vertices under
// that (domain, scope). OrderMoveDomScope instances are themselves organized into a global ready
// list if they have ready vertices.
class ترتیب_Move_Dom_Scope final {
    // STATE
    ترتیب_Move_راس::List m_readyVertices;  // Ready vertices in this domain/scope
    V3ListLinks<ترتیب_Move_Dom_Scope> m_links;  // List links to store instances of this class
    bool m_isOnList = false;  // True if DomScope is already on a list through m_listEnt
    ثابت ترتیب_Domain* ثابت m_domainp;  // Domain the vertices belong to
    ثابت AstScope* ثابت m_scopep;  // Scope the vertices belong to

    // Key type for map below
    class Dom_Scope_Map_کلید final {
        ثابت ترتیب_Domain* ثابت m_domainp;  // Domain the vertices belong to
        ثابت AstScope* ثابت m_scopep;  // Scope the vertices belong to

    عام:
        Dom_Scope_Map_کلید(ثابت ترتیب_Domain* domainp, ثابت AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        بنية Hash final {
            ناپ_t عامل()(ثابت Dom_Scope_Map_کلید& کلید) ثابت {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{کلید.m_domainp};
                hash += کلید.m_scopep;
                أرجع hash.قدر();
            }
        };

        بنية Equal final {
            bool عامل()(ثابت Dom_Scope_Map_کلید& a, ثابت Dom_Scope_Map_کلید& b) ثابت {
                أرجع a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_کلید, ترتیب_Move_Dom_Scope, Dom_Scope_Map_کلید::Hash,
                                           Dom_Scope_Map_کلید::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ساكن DomScopeMap s_dsMap;

    V3ListLinks<ترتیب_Move_Dom_Scope>& links() { أرجع m_links; }

عام:
    // List type to store instances of this class
    using List = V3List<ترتیب_Move_Dom_Scope, &ترتیب_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    ساكن ترتیب_Move_Dom_Scope& getOrCreate(ثابت ترتیب_Domain* domainp, ثابت AstScope* scopep) {
        أرجع s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .پہلا->دوجا;
    }
    ساكن فراغ clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    ترتیب_Move_Dom_Scope(ثابت ترتیب_Domain* domainp, ثابت AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~ترتیب_Move_Dom_Scope() = افتراضي;
    VL_UNCOPYABLE(ترتیب_Move_Dom_Scope);
    VL_UNMOVABLE(ترتیب_Move_Dom_Scope);

    // MEMBERS
    ترتیب_Move_راس::List& readyVertices() { أرجع m_readyVertices; }
    ثابت ترتیب_Domain* domainp() ثابت { أرجع m_domainp; }
    ثابت AstScope* scopep() ثابت { أرجع m_scopep; }

    bool isOnList() ثابت { أرجع m_isOnList; }
    فراغ isOnList(bool قدر) { m_isOnList = قدر; }
};

//======================================================================
// Serializer for OrderMoveGraph

class ترتیب_Move_گراف_Serializer final {
    // STATE
    ترتیب_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    ترتیب_Move_Dom_Scope* m_اگلا_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    فراغ ready(ترتیب_Move_راس* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        إذا (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            ترتیب_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_پٹھ(vtxp);
            // Add the DomScope to the global ready list if not there yet
            إذا (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_پٹھ(&domScope);
            }
        } وإلا {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            لكل (V3_گراف_کنارا& کنارا : vtxp->باہرلا_کنارے()) {
                // The dependent variable
                ترتیب_Move_راس* ثابت dVtxp = کنارا.چوٹی()->as<ترتیب_Move_راس>();
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
    explicit ترتیب_Move_گراف_Serializer(ترتیب_Move_گراف& move_گراف) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        لكل (V3_گراف_راس& vtx : move_گراف.vertices()) {
            ثابت uint32_t nDeps = vtx.وچ_کنارے().ناپ();
            vtx.user(nDeps);
        }
    }
    ~ترتیب_Move_گراف_Serializer() = افتراضي;
    VL_UNCOPYABLE(ترتیب_Move_گراف_Serializer);

    // Add a seed vertex to the ready lists
    فراغ جوڑو_بیج(ترتیب_Move_راس* vtxp) { ready(vtxp); }

    ترتیب_Move_راس* get_اگلا() {
        إذا (!m_اگلا_Dom_Scopep) m_اگلا_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        إذا (!m_اگلا_Dom_Scopep) أرجع nullptr;
        ترتیب_Move_Dom_Scope& currDomScope = *m_اگلا_Dom_Scopep;

        ترتیب_Move_راس::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.خالی_تھاواں(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        ترتیب_Move_راس* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        إذا (currReadyList.خالی_تھاواں()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        لكل (V3_گراف_کنارا& کنارا : mVtxp->باہرلا_کنارے()) {
            // The dependent variable
            ترتیب_Move_راس* ثابت dVtxp = کنارا.چوٹی()->as<ترتیب_Move_راس>();
            // Update number of dependencies
            ثابت uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            إذا (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        إذا (currReadyList.خالی_تھاواں()) {
            m_اگلا_Dom_Scopep = nullptr;
            لكل (ترتیب_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                إذا (domScope.domainp() == currDomScope.domainp()) {
                    m_اگلا_Dom_Scopep = &domScope;
                    اكسر;
                }
            }
        }

        // Finally yield the selected vertex
        أرجع mVtxp;
    }
};

#جے_انت  // Guard
