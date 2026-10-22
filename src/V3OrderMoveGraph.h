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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ORDERMOVEGRAPH_H_
#تعریف_کرو VERILATOR_V3ORDERMOVEGRAPH_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Graph.h"
#شامل "V3Order.h"
#شامل "V3OrderGraph.h"

class ترتیب_Move_Dom_Scope;
class ترتیب_Move_گراف;

//======================================================================
// Vertex types

class ترتیب_Move_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(ترتیب_Move_راس, V3_گراف_راس)

    // The corresponding logic vertex, or nullptr if this MoveVertex stands for a variable vertex.
    ترتیب_Logic_راس* ثابت m_logicp;
    ترتیب_Move_Dom_Scope& m_domScope;  // DomScope this vertex is under
    V3ListLinks<ترتیب_Move_راس> m_links;  // List links to store instances of this class

    // METHODS
    std::string dotColor() ثابت override { واپس logicp() ? logicp()->dotColor() : "yellow"; }

    std::string name() ثابت override VL_MT_STABLE {
        std::string nm;
        اگر (!logicp()) {
            nm = "var";
        } ورنہ {
            nm = logicp()->name() + "\\n";
            nm += "MV:";
            nm += +" d=" + cvt_مقصد_Hex(logicp()->domainp());
            nm += +" s=" + cvt_مقصد_Hex(logicp()->scopep());
        }
        اگر (userp()) nm += "\nu=" + cvt_مقصد_Hex(userp());
        واپس nm;
    }

    V3ListLinks<ترتیب_Move_راس>& links() { واپس m_links; }

عوامی:
    // List type to store instances of this class
    using List = V3List<ترتیب_Move_راس, &ترتیب_Move_راس::links>;

    // CONSTRUCTORS
    ترتیب_Move_راس(ترتیب_Move_گراف& گراف, ترتیب_Logic_راس* lVtxp,
                    ثابت ترتیب_Domain* domainp) VL_MT_DISABLED;
    ~ترتیب_Move_راس() override = طےشدہ;
    VL_UNCOPYABLE(ترتیب_Move_راس);

    ترتیب_Logic_راس* logicp() ثابت VL_MT_STABLE { واپس m_logicp; }
    ترتیب_Move_Dom_Scope& domScope() ثابت { واپس m_domScope; }
};

//======================================================================
// Graph type

// OrderMoveGraph is constructed from the fine-grained OrderGraph.
// It is a slightly coarsened representation of dependencies used to drive serialization.
class ترتیب_Move_گراف final : عوامی V3_گراف {
    ترتیب_گراف& m_ترتیب_گراف;  // The OrderGraph this move graph was built from

عوامی:
    explicit ترتیب_Move_گراف(ترتیب_گراف& ترتیب_گراف_2)
        : m_ترتیب_گراف{ترتیب_گراف_2} {}

    ترتیب_گراف& ترتیب_گراف_2() ثابت { واپس m_ترتیب_گراف; }

    // Build an OrderMoveGraph from an OrderGraph
    ساکن std::unique_ptr<ترتیب_Move_گراف> build(ترتیب_گراف&);
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

    عوامی:
        Dom_Scope_Map_کلید(ثابت ترتیب_Domain* domainp, ثابت AstScope* scopep)
            : m_domainp{domainp}
            , m_scopep{scopep} {}

        ساخت Hash final {
            سائز_t عامل()(ثابت Dom_Scope_Map_کلید& کلید) ثابت {
                // cppcheck-suppress unreadVariable  // cppcheck bug
                V3Hash hash{کلید.m_domainp};
                hash += کلید.m_scopep;
                واپس hash.قدر();
            }
        };

        ساخت Equal final {
            bool عامل()(ثابت Dom_Scope_Map_کلید& a, ثابت Dom_Scope_Map_کلید& b) ثابت {
                واپس a.m_domainp == b.m_domainp && a.m_scopep == b.m_scopep;
            }
        };
    };

    using DomScopeMap = std::unordered_map<Dom_Scope_Map_کلید, ترتیب_Move_Dom_Scope, Dom_Scope_Map_کلید::Hash,
                                           Dom_Scope_Map_کلید::Equal>;
    // Map from Domain/Scope to the corresponding DomScope instance
    ساکن DomScopeMap s_dsMap;

    V3ListLinks<ترتیب_Move_Dom_Scope>& links() { واپس m_links; }

عوامی:
    // List type to store instances of this class
    using List = V3List<ترتیب_Move_Dom_Scope, &ترتیب_Move_Dom_Scope::links>;

    // STATIC MEMBERS
    ساکن ترتیب_Move_Dom_Scope& getOrCreate(ثابت ترتیب_Domain* domainp, ثابت AstScope* scopep) {
        واپس s_dsMap
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(domainp, scopep),  //
                     std::forward_as_tuple(domainp, scopep))
            .پہلا->دوسرا;
    }
    ساکن خالی clear() { s_dsMap.clear(); }

    // CONSTRUCTOR
    ترتیب_Move_Dom_Scope(ثابت ترتیب_Domain* domainp, ثابت AstScope* scopep)
        : m_domainp{domainp}
        , m_scopep{scopep} {}
    ~ترتیب_Move_Dom_Scope() = طےشدہ;
    VL_UNCOPYABLE(ترتیب_Move_Dom_Scope);
    VL_UNMOVABLE(ترتیب_Move_Dom_Scope);

    // MEMBERS
    ترتیب_Move_راس::List& readyVertices() { واپس m_readyVertices; }
    ثابت ترتیب_Domain* domainp() ثابت { واپس m_domainp; }
    ثابت AstScope* scopep() ثابت { واپس m_scopep; }

    bool isOnList() ثابت { واپس m_isOnList; }
    خالی isOnList(bool قدر) { m_isOnList = قدر; }
};

//======================================================================
// Serializer for OrderMoveGraph

class ترتیب_Move_گراف_Serializer final {
    // STATE
    ترتیب_Move_Dom_Scope::List m_readyDomScopeps;  // List of DomScopes which have ready vertices
    ترتیب_Move_Dom_Scope* m_اگلا_Dom_Scopep = nullptr;  // Next DomScope to yield from

    // METHODS

    خالی ready(ترتیب_Move_راس* vtxp) {
        UASSERT_OBJ(!vtxp->user(), vtxp, "'ready' called on vertex with pending dependencies");
        اگر (vtxp->logicp()) {
            // Add this vertex to the ready list of its DomScope
            ترتیب_Move_Dom_Scope& domScope = vtxp->domScope();
            domScope.readyVertices().link_عقب(vtxp);
            // Add the DomScope to the global ready list if not there yet
            اگر (!domScope.isOnList()) {
                domScope.isOnList(true);
                m_readyDomScopeps.link_عقب(&domScope);
            }
        } ورنہ {  // This is a bit nonsense at this point, but equivalent to the old version
            // Remove dependency on vertex we are returning. This might add vertices to
            // currReadyList.
            برائے (V3_گراف_کنارہ& کنارہ : vtxp->برآمد_کنارے()) {
                // The dependent variable
                ترتیب_Move_راس* ثابت dVtxp = کنارہ.چوٹی()->as<ترتیب_Move_راس>();
                // Update number of dependencies
                ثابت uint32_t nDeps = dVtxp->user() - 1;
                dVtxp->user(nDeps);
                // If no more dependencies, mark it ready
                اگر (!nDeps) ready(dVtxp);
            }
        }
    }

عوامی:
    // CONSTRUCTOR
    explicit ترتیب_Move_گراف_Serializer(ترتیب_Move_گراف& move_گراف) {
        // Set V3GraphVertex::user() to the number of incoming edges (upstream dependencies)
        برائے (V3_گراف_راس& vtx : move_گراف.vertices()) {
            ثابت uint32_t nDeps = vtx.اندر_کنارے().سائز();
            vtx.user(nDeps);
        }
    }
    ~ترتیب_Move_گراف_Serializer() = طےشدہ;
    VL_UNCOPYABLE(ترتیب_Move_گراف_Serializer);

    // Add a seed vertex to the ready lists
    خالی جوڑو_بیج(ترتیب_Move_راس* vtxp) { ready(vtxp); }

    ترتیب_Move_راس* get_اگلا() {
        اگر (!m_اگلا_Dom_Scopep) m_اگلا_Dom_Scopep = m_readyDomScopeps.frontp();
        // If nothing is ready, we are done
        اگر (!m_اگلا_Dom_Scopep) واپس nullptr;
        ترتیب_Move_Dom_Scope& currDomScope = *m_اگلا_Dom_Scopep;

        ترتیب_Move_راس::List& currReadyList = currDomScope.readyVertices();
        UASSERT(!currReadyList.خالی_خانے(), "DomScope on ready list, but has no ready vertices");

        // Remove vertex from ready list under the DomScope. This is the vertex we are returning.
        ترتیب_Move_راس* mVtxp = currReadyList.unlinkFront();

        // Nonsesne, but what we used to do
        اگر (currReadyList.خالی_خانے()) {
            currDomScope.isOnList(false);
            m_readyDomScopeps.unlink(&currDomScope);
        }

        // Remove dependency on vertex we are returning. This might add vertices to currReadyList.
        برائے (V3_گراف_کنارہ& کنارہ : mVtxp->برآمد_کنارے()) {
            // The dependent variable
            ترتیب_Move_راس* ثابت dVtxp = کنارہ.چوٹی()->as<ترتیب_Move_راس>();
            // Update number of dependencies
            ثابت uint32_t nDeps = dVtxp->user() - 1;
            dVtxp->user(nDeps);
            // If no more dependencies, mark it ready
            اگر (!nDeps) ready(dVtxp);
        }

        // If no more ready vertices in the current DomScope, prefer to continue with a new scope
        // under the same domain.
        اگر (currReadyList.خالی_خانے()) {
            m_اگلا_Dom_Scopep = nullptr;
            برائے (ترتیب_Move_Dom_Scope& domScope : m_readyDomScopeps) {
                اگر (domScope.domainp() == currDomScope.domainp()) {
                    m_اگلا_Dom_Scopep = &domScope;
                    توڑو;
                }
            }
        }

        // Finally yield the selected vertex
        واپس mVtxp;
    }
};

#اگر_ختم  // Guard
